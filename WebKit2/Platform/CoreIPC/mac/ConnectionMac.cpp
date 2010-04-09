/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Connection.h"

#include "MachPort.h"
#include "RunLoop.h"

using namespace std;

namespace CoreIPC {

static const size_t inlineMessageMaxSize = 4096;

// FIXME: Share this between ConnectionMac.cpp and ConnectionWin.cpp.
namespace CoreIPCMessage {

enum Kind {
    InitializeConnection,
    SyncMessageReply,
};

}

template<> struct MessageKindTraits<CoreIPCMessage::Kind> { 
    static const MessageClass messageClass = MessageClassCoreIPC;
};

void Connection::platformInvalidate()
{
    if (!m_isConnected)
        return;

    m_isConnected = false;

    ASSERT(m_sendPort);
    ASSERT(m_receivePort);
    
    // Unregister our ports.
    m_connectionQueue.unregisterMachPortEventHandler(m_sendPort);
    m_sendPort = MACH_PORT_NULL;

    m_connectionQueue.unregisterMachPortEventHandler(m_receivePort);
    m_receivePort = MACH_PORT_NULL;
}

void Connection::platformInitialize(Identifier identifier)
{
    if (m_isServer) {
        m_receivePort = identifier;
        m_sendPort = MACH_PORT_NULL;
    } else {
        m_receivePort = MACH_PORT_NULL;
        m_sendPort = identifier;
    }
}

bool Connection::open()
{
    if (m_isServer) {
        ASSERT(m_receivePort);
        ASSERT(!m_sendPort);
        
    } else {
        ASSERT(!m_receivePort);
        ASSERT(m_sendPort);

        // Create the receive port.
        mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &m_receivePort);

        // Send the initialize message, which contains a send right for the server to use.
        send(CoreIPCMessage::InitializeConnection, 0, MachPort(m_receivePort, MACH_MSG_TYPE_MAKE_SEND));

        // We're now connected.
        m_isConnected = true;
        
        // Set the dead name handler for our send port.
        initializeDeadNameSource();
    }
    
    // Register the data available handler.
    m_connectionQueue.registerMachPortEventHandler(m_receivePort, WorkQueue::MachPortDataAvailable, 
                                                   WorkItem::create(this, &Connection::receiveSourceEventHandler));

    return true;
}

static inline size_t machMessageSize(size_t bodySize, size_t numberOfPortDescriptors = 0, size_t numberOfOOLMemoryDescriptors = 0)
{
    size_t size = sizeof(mach_msg_header_t) + bodySize;
    if (numberOfPortDescriptors || numberOfOOLMemoryDescriptors) {
        size += sizeof(mach_msg_body_t);
        if (numberOfPortDescriptors)
            size += (numberOfPortDescriptors * sizeof(mach_msg_port_descriptor_t));
        if (numberOfOOLMemoryDescriptors)
            size += (numberOfOOLMemoryDescriptors * sizeof(mach_msg_ool_ports_descriptor_t));
    }
    return round_msg(size);
}

void Connection::sendOutgoingMessage(unsigned messageID, auto_ptr<ArgumentEncoder> arguments)
{
    std::list<Attachment> attachments = arguments->releaseAttachments();
    
    size_t numberOfPortDescriptors = 0;
    size_t numberOfOOLMemoryDescriptors = 0;
    for (std::list<Attachment>::const_iterator it = attachments.begin(), end = attachments.end(); it != end; ++it) {
        Attachment::Type type = ((Attachment)*it).type();
        if (type == Attachment::MachPortType)
            numberOfPortDescriptors++;
        else if (type == Attachment::MachOOLMemoryType)
            numberOfOOLMemoryDescriptors++;
    }
    
    size_t messageSize = machMessageSize(arguments->bufferSize(), numberOfPortDescriptors, numberOfOOLMemoryDescriptors);
    
    char buffer[inlineMessageMaxSize];

    if (messageSize > sizeof(buffer)) {
        // FIXME: The message should be OOL.
        ASSERT(false);
    }

    bool isComplex = (numberOfPortDescriptors + numberOfOOLMemoryDescriptors > 0);

    mach_msg_header_t* header = reinterpret_cast<mach_msg_header_t*>(&buffer);
    header->msgh_bits = isComplex ? MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND | MACH_MSGH_BITS_COMPLEX, 0) : MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header->msgh_size = messageSize;
    header->msgh_remote_port = m_sendPort;
    header->msgh_local_port = MACH_PORT_NULL;
    header->msgh_id = messageID;

    uint8_t* messageData;

    if (isComplex) {
        mach_msg_body_t* body = reinterpret_cast<mach_msg_body_t*>(header + 1);
        body->msgh_descriptor_count = numberOfPortDescriptors + numberOfOOLMemoryDescriptors;

        uint8_t* descriptorData = reinterpret_cast<uint8_t*>(body + 1);
        for (std::list<Attachment>::const_iterator it = attachments.begin(), end = attachments.end(); it != end; ++it) {
            Attachment attachment = *it;

            mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);
            switch (attachment.type()) {
                case Attachment::MachPortType:
                    descriptor->port.name = attachment.port();
                    descriptor->port.disposition = attachment.disposition();
                    descriptor->port.type = MACH_MSG_PORT_DESCRIPTOR;            

                    descriptorData += sizeof(mach_msg_port_descriptor_t);
                    break;
                case Attachment::MachOOLMemoryType:
                    descriptor->out_of_line.address = attachment.address();
                    descriptor->out_of_line.size = attachment.size();
                    descriptor->out_of_line.copy = attachment.copyOptions();
                    descriptor->out_of_line.deallocate = attachment.deallocate();
                    descriptor->out_of_line.type = MACH_MSG_OOL_DESCRIPTOR;            

                    descriptorData += sizeof(mach_msg_ool_descriptor_t);
                    break;
                default:
                    ASSERT_NOT_REACHED();
            }
        }

        messageData = descriptorData;
    } else
        messageData = (uint8_t*)(header + 1);

    // Copy the data.
    memcpy(messageData, arguments->buffer(), arguments->bufferSize());

    ASSERT(m_sendPort);
    
    // Send the message.
    kern_return_t kr = mach_msg(header, MACH_SEND_MSG, messageSize, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (kr != KERN_SUCCESS) {
        // FIXME: What should we do here?
        return;
    }
}

void Connection::initializeDeadNameSource()
{
    m_connectionQueue.registerMachPortEventHandler(m_sendPort, WorkQueue::MachPortDeadNameNotification, WorkItem::create(this, &Connection::connectionDidClose));
}

static auto_ptr<ArgumentDecoder> createArgumentDecoder(mach_msg_header_t* header)
{
    if (!(header->msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
        // We have a simple message.
        size_t bodySize = header->msgh_size - sizeof(mach_msg_header_t);
        uint8_t* body = reinterpret_cast<uint8_t*>(header + 1);
        
        return auto_ptr<ArgumentDecoder>(new ArgumentDecoder(body, bodySize));
    }

    mach_msg_body_t* body = reinterpret_cast<mach_msg_body_t*>(header + 1);
    mach_msg_size_t numDescriptors = body->msgh_descriptor_count;
    ASSERT(numDescriptors);

    // Build attachment list
    std::queue<Attachment> attachments;
    uint8_t* descriptorData = reinterpret_cast<uint8_t*>(body + 1);
    for (mach_msg_size_t i = 0; i < numDescriptors; ++i) {
        mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);

        switch (descriptor->type.type) {
            case MACH_MSG_PORT_DESCRIPTOR:
                attachments.push(Attachment(descriptor->port.name, descriptor->port.disposition));
                descriptorData += sizeof(mach_msg_port_descriptor_t);
                break;
            case MACH_MSG_OOL_DESCRIPTOR:
                attachments.push(Attachment(descriptor->out_of_line.address, descriptor->out_of_line.size,
                                            descriptor->out_of_line.copy, descriptor->out_of_line.deallocate));
                descriptorData += sizeof(mach_msg_ool_descriptor_t);
                break;
            default:
                ASSERT(false && "Unhandled descriptor type");
        }
    }

    uint8_t* messageBody = descriptorData;
    size_t messageBodySize = header->msgh_size - (descriptorData - reinterpret_cast<uint8_t*>(header));

    return auto_ptr<ArgumentDecoder>(new ArgumentDecoder(messageBody, messageBodySize, attachments));
}

void Connection::receiveSourceEventHandler()
{
    char buffer[inlineMessageMaxSize];
    
    mach_msg_header_t* header = reinterpret_cast<mach_msg_header_t*>(&buffer);
    
    kern_return_t kr = mach_msg(header, MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT, 0, sizeof(buffer), m_receivePort, 0, MACH_PORT_NULL);
    if (kr == MACH_RCV_TIMED_OUT)
        return;

    if (kr != MACH_MSG_SUCCESS) {

        ASSERT_NOT_REACHED();
        // FIXME: Handle MACH_RCV_MSG_TOO_LARGE.
        return;
    }
    
    MessageID messageID = MessageID::fromInt(header->msgh_id);
    std::auto_ptr<ArgumentDecoder> arguments = createArgumentDecoder(header);
    ASSERT(arguments.get());

    if (messageID == MessageID(CoreIPCMessage::InitializeConnection)) {
        ASSERT(m_isServer);
        ASSERT(!m_isConnected);
        ASSERT(!m_sendPort);

        MachPort port;
        if (!arguments->decode(port)) {
            // FIXME: Disconnect.
            return;
        }

        m_sendPort = port.port();
        
        // Set the dead name source if needed.
        if (m_sendPort)
            initializeDeadNameSource();
        
        m_isConnected = true;

        // Send any pending outgoing messages.
        sendOutgoingMessages();
        
        return;
    }
    
    processIncomingMessage(messageID, arguments);
}    

} // namespace CoreIPC
