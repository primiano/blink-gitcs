// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "modules/websockets/WebSocketChannel.h"

#include "core/fileapi/Blob.h"
#include "core/frame/ConsoleTypes.h"
#include "core/testing/DummyPageHolder.h"
#include "modules/websockets/NewWebSocketChannelImpl.h"
#include "modules/websockets/WebSocketChannelClient.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "public/platform/WebSerializedOrigin.h"
#include "public/platform/WebSocketHandle.h"
#include "public/platform/WebSocketHandleClient.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebVector.h"
#include "wtf/ArrayBuffer.h"
#include "wtf/OwnPtr.h"
#include "wtf/Vector.h"
#include "wtf/text/WTFString.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::InSequence;
using testing::PrintToString;
using testing::AnyNumber;


namespace WebCore {

namespace {

typedef testing::StrictMock< testing::MockFunction<void(int)> > Checkpoint;

class MockWebSocketChannelClient : public RefCountedWillBeRefCountedGarbageCollected<MockWebSocketChannelClient>, public WebSocketChannelClient {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(MockWebSocketChannelClient);
public:
    static PassRefPtrWillBeRawPtr<MockWebSocketChannelClient> create()
    {
        return adoptRefWillBeRefCountedGarbageCollected(new testing::StrictMock<MockWebSocketChannelClient>());
    }

    MockWebSocketChannelClient() { }

    virtual ~MockWebSocketChannelClient() { }

    MOCK_METHOD2(didConnect, void(const String&, const String&));
    MOCK_METHOD1(didReceiveMessage, void(const String&));
    virtual void didReceiveBinaryData(PassOwnPtr<Vector<char> > binaryData) OVERRIDE
    {
        didReceiveBinaryDataMock(*binaryData);
    }
    MOCK_METHOD1(didReceiveBinaryDataMock, void(const Vector<char>&));
    MOCK_METHOD0(didReceiveMessageError, void());
    MOCK_METHOD1(didConsumeBufferedAmount, void(unsigned long));
    MOCK_METHOD0(didStartClosingHandshake, void());
    MOCK_METHOD3(didClose, void(ClosingHandshakeCompletionStatus, unsigned short, const String&));
    MOCK_METHOD1(trace, void(Visitor*));
};

class MockWebSocketHandle : public blink::WebSocketHandle {
public:
    static MockWebSocketHandle* create()
    {
        return new testing::StrictMock<MockWebSocketHandle>();
    }

    MockWebSocketHandle() { }

    virtual ~MockWebSocketHandle() { }

    MOCK_METHOD4(connect, void(const blink::WebURL&, const blink::WebVector<blink::WebString>&, const blink::WebSerializedOrigin&, blink::WebSocketHandleClient*));
    MOCK_METHOD4(send, void(bool, blink::WebSocketHandle::MessageType, const char*, size_t));
    MOCK_METHOD1(flowControl, void(int64_t));
    MOCK_METHOD2(close, void(unsigned short, const blink::WebString&));
};

class NewWebSocketChannelImplTest : public ::testing::Test {
public:
    NewWebSocketChannelImplTest()
        : m_pageHolder(DummyPageHolder::create())
        , m_channelClient(MockWebSocketChannelClient::create())
        , m_handle(MockWebSocketHandle::create())
        , m_channel(NewWebSocketChannelImpl::create(&m_pageHolder->document(), m_channelClient.get(), String(), 0, handle()))
        , m_sumOfConsumedBufferedAmount(0)
    {
        ON_CALL(*channelClient(), didConsumeBufferedAmount(_)).WillByDefault(Invoke(this, &NewWebSocketChannelImplTest::didConsumeBufferedAmount));
    }

    ~NewWebSocketChannelImplTest() { }

    MockWebSocketChannelClient* channelClient()
    {
        return m_channelClient.get();
    }

    WebSocketChannel* channel()
    {
        return static_cast<WebSocketChannel*>(m_channel.get());
    }

    blink::WebSocketHandleClient* handleClient()
    {
        return static_cast<blink::WebSocketHandleClient*>(m_channel.get());
    }

    MockWebSocketHandle* handle()
    {
        return m_handle;
    }

    void didConsumeBufferedAmount(unsigned long a)
    {
        m_sumOfConsumedBufferedAmount += a;
    }

    void connect()
    {
        {
            InSequence s;
            EXPECT_CALL(*handle(), connect(blink::WebURL(KURL(KURL(), "ws://localhost/")), _, _, handleClient()));
            EXPECT_CALL(*handle(), flowControl(65536));
            EXPECT_CALL(*channelClient(), didConnect(String("a"), String("b")));
        }
        EXPECT_TRUE(channel()->connect(KURL(KURL(), "ws://localhost/"), "x"));
        handleClient()->didConnect(handle(), false, blink::WebString("a"), blink::WebString("b"));
        ::testing::Mock::VerifyAndClearExpectations(this);
    }

    OwnPtr<DummyPageHolder> m_pageHolder;
    RefPtrWillBePersistent<MockWebSocketChannelClient> m_channelClient;
    MockWebSocketHandle* m_handle;
    RefPtrWillBePersistent<NewWebSocketChannelImpl> m_channel;
    unsigned long m_sumOfConsumedBufferedAmount;
};

MATCHER_P2(MemEq, p, len,
    std::string("pointing to memory")
    + (negation ? " not" : "")
    + " equal to \""
    + std::string(p, len) + "\" (length=" + PrintToString(len) + ")"
)
{
    return memcmp(arg, p, len) == 0;
}

TEST_F(NewWebSocketChannelImplTest, connectSuccess)
{
    Checkpoint checkpoint;
    {
        InSequence s;
        EXPECT_CALL(*handle(), connect(blink::WebURL(KURL(KURL(), "ws://localhost/")), _, _, handleClient()));
        EXPECT_CALL(*handle(), flowControl(65536));
        EXPECT_CALL(checkpoint, Call(1));
        EXPECT_CALL(*channelClient(), didConnect(String("a"), String("b")));
    }

    EXPECT_TRUE(channel()->connect(KURL(KURL(), "ws://localhost/"), "x"));
    checkpoint.Call(1);
    handleClient()->didConnect(handle(), false, blink::WebString("a"), blink::WebString("b"));
}

TEST_F(NewWebSocketChannelImplTest, sendText)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeText, MemEq("foo", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeText, MemEq("bar", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeText, MemEq("baz", 3), 3));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send("foo"));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send("bar"));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send("baz"));

    EXPECT_EQ(9ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendTextContinuation)
{
    connect();
    Checkpoint checkpoint;
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(false, blink::WebSocketHandle::MessageTypeText, MemEq("0123456789abcdef", 16), 16));
        EXPECT_CALL(checkpoint, Call(1));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeContinuation, MemEq("g", 1), 1));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeText, MemEq("hijk", 4), 4));
        EXPECT_CALL(*handle(), send(false, blink::WebSocketHandle::MessageTypeText, MemEq("lmnopqrstuv", 11), 11));
        EXPECT_CALL(checkpoint, Call(2));
        EXPECT_CALL(*handle(), send(false, blink::WebSocketHandle::MessageTypeContinuation, MemEq("wxyzABCDEFGHIJKL", 16), 16));
        EXPECT_CALL(checkpoint, Call(3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeContinuation, MemEq("MNOPQRSTUVWXYZ", 14), 14));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send("0123456789abcdefg"));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send("hijk"));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send("lmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    checkpoint.Call(1);
    handleClient()->didReceiveFlowControl(handle(), 16);
    checkpoint.Call(2);
    handleClient()->didReceiveFlowControl(handle(), 16);
    checkpoint.Call(3);
    handleClient()->didReceiveFlowControl(handle(), 16);

    EXPECT_EQ(62ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendTextNonLatin1)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeText, MemEq("\xe7\x8b\x90\xe0\xa4\x94", 6), 6));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    UChar nonLatin1String[] = {
        0x72d0,
        0x0914,
        0x0000
    };
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(nonLatin1String));

    EXPECT_EQ(6ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendTextNonLatin1Continuation)
{
    connect();
    Checkpoint checkpoint;
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(false, blink::WebSocketHandle::MessageTypeText, MemEq("\xe7\x8b\x90\xe0\xa4\x94\xe7\x8b\x90\xe0\xa4\x94\xe7\x8b\x90\xe0", 16), 16));
        EXPECT_CALL(checkpoint, Call(1));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeContinuation, MemEq("\xa4\x94", 2), 2));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    UChar nonLatin1String[] = {
        0x72d0,
        0x0914,
        0x72d0,
        0x0914,
        0x72d0,
        0x0914,
        0x0000
    };
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(nonLatin1String));
    checkpoint.Call(1);
    handleClient()->didReceiveFlowControl(handle(), 16);

    EXPECT_EQ(18ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInVector)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("foo", 3), 3));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    Vector<char> fooVector;
    fooVector.append("foo", 3);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(fooVector))));

    EXPECT_EQ(3ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInVectorWithNullBytes)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\0ar", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("b\0z", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("qu\0", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\0\0\0", 3), 3));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    {
        Vector<char> v;
        v.append("\0ar", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));
    }
    {
        Vector<char> v;
        v.append("b\0z", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));
    }
    {
        Vector<char> v;
        v.append("qu\0", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));
    }
    {
        Vector<char> v;
        v.append("\0\0\0", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));
    }

    EXPECT_EQ(12ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInVectorNonLatin1UTF8)
{
    connect();
    EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\xe7\x8b\x90", 3), 3));

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    Vector<char> v;
    v.append("\xe7\x8b\x90", 3);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));

    EXPECT_EQ(3ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInVectorNonUTF8)
{
    connect();
    EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\x80\xff\xe7", 3), 3));

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    Vector<char> v;
    v.append("\x80\xff\xe7", 3);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));

    EXPECT_EQ(3ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInVectorNonLatin1UTF8Continuation)
{
    connect();
    Checkpoint checkpoint;
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(false, blink::WebSocketHandle::MessageTypeBinary, MemEq("\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7", 16), 16));
        EXPECT_CALL(checkpoint, Call(1));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeContinuation, MemEq("\x8b\x90", 2), 2));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    Vector<char> v;
    v.append("\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90", 18);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(adoptPtr(new Vector<char>(v))));
    checkpoint.Call(1);

    handleClient()->didReceiveFlowControl(handle(), 16);

    EXPECT_EQ(18ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInArrayBuffer)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("foo", 3), 3));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    RefPtr<ArrayBuffer> fooBuffer = ArrayBuffer::create("foo", 3);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*fooBuffer, 0, 3));

    EXPECT_EQ(3ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInArrayBufferPartial)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("foo", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("bar", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("baz", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("a", 1), 1));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    RefPtr<ArrayBuffer> foobarBuffer = ArrayBuffer::create("foobar", 6);
    RefPtr<ArrayBuffer> qbazuxBuffer = ArrayBuffer::create("qbazux", 6);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*foobarBuffer, 0, 3));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*foobarBuffer, 3, 3));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*qbazuxBuffer, 1, 3));
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*qbazuxBuffer, 2, 1));

    EXPECT_EQ(10ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInArrayBufferWithNullBytes)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\0ar", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("b\0z", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("qu\0", 3), 3));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\0\0\0", 3), 3));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    {
        RefPtr<ArrayBuffer> b = ArrayBuffer::create("\0ar", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 3));
    }
    {
        RefPtr<ArrayBuffer> b = ArrayBuffer::create("b\0z", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 3));
    }
    {
        RefPtr<ArrayBuffer> b = ArrayBuffer::create("qu\0", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 3));
    }
    {
        RefPtr<ArrayBuffer> b = ArrayBuffer::create("\0\0\0", 3);
        EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 3));
    }

    EXPECT_EQ(12ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInArrayBufferNonLatin1UTF8)
{
    connect();
    EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\xe7\x8b\x90", 3), 3));

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    RefPtr<ArrayBuffer> b = ArrayBuffer::create("\xe7\x8b\x90", 3);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 3));

    EXPECT_EQ(3ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInArrayBufferNonUTF8)
{
    connect();
    EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeBinary, MemEq("\x80\xff\xe7", 3), 3));

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    RefPtr<ArrayBuffer> b = ArrayBuffer::create("\x80\xff\xe7", 3);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 3));

    EXPECT_EQ(3ul, m_sumOfConsumedBufferedAmount);
}

TEST_F(NewWebSocketChannelImplTest, sendBinaryInArrayBufferNonLatin1UTF8Continuation)
{
    connect();
    Checkpoint checkpoint;
    {
        InSequence s;
        EXPECT_CALL(*handle(), send(false, blink::WebSocketHandle::MessageTypeBinary, MemEq("\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7", 16), 16));
        EXPECT_CALL(checkpoint, Call(1));
        EXPECT_CALL(*handle(), send(true, blink::WebSocketHandle::MessageTypeContinuation, MemEq("\x8b\x90", 2), 2));
    }

    handleClient()->didReceiveFlowControl(handle(), 16);
    EXPECT_CALL(*channelClient(), didConsumeBufferedAmount(_)).Times(AnyNumber());

    RefPtr<ArrayBuffer> b = ArrayBuffer::create("\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90\xe7\x8b\x90", 18);
    EXPECT_EQ(WebSocketChannel::SendSuccess, channel()->send(*b, 0, 18));
    checkpoint.Call(1);

    handleClient()->didReceiveFlowControl(handle(), 16);

    EXPECT_EQ(18ul, m_sumOfConsumedBufferedAmount);
}

// FIXME: Add tests for WebSocketChannel::send(PassRefPtr<BlobDataHandle>)

TEST_F(NewWebSocketChannelImplTest, receiveText)
{
    connect();
    {
        InSequence s;
        EXPECT_CALL(*channelClient(), didReceiveMessage(String("FOO")));
        EXPECT_CALL(*channelClient(), didReceiveMessage(String("BAR")));
    }

    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeText, "FOOX", 3);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeText, "BARX", 3);
}

TEST_F(NewWebSocketChannelImplTest, receiveTextContinuation)
{
    connect();
    EXPECT_CALL(*channelClient(), didReceiveMessage(String("BAZ")));

    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeText, "BX", 1);
    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeContinuation, "AX", 1);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeContinuation, "ZX", 1);
}

TEST_F(NewWebSocketChannelImplTest, receiveTextNonLatin1)
{
    connect();
    UChar nonLatin1String[] = {
        0x72d0,
        0x0914,
        0x0000
    };
    EXPECT_CALL(*channelClient(), didReceiveMessage(String(nonLatin1String)));

    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeText, "\xe7\x8b\x90\xe0\xa4\x94", 6);
}

TEST_F(NewWebSocketChannelImplTest, receiveTextNonLatin1Continuation)
{
    connect();
    UChar nonLatin1String[] = {
        0x72d0,
        0x0914,
        0x0000
    };
    EXPECT_CALL(*channelClient(), didReceiveMessage(String(nonLatin1String)));

    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeText, "\xe7\x8b", 2);
    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeContinuation, "\x90\xe0", 2);
    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeContinuation, "\xa4", 1);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeContinuation, "\x94", 1);
}

TEST_F(NewWebSocketChannelImplTest, receiveBinary)
{
    connect();
    Vector<char> fooVector;
    fooVector.append("FOO", 3);
    EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(fooVector));

    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "FOOx", 3);
}

TEST_F(NewWebSocketChannelImplTest, receiveBinaryContinuation)
{
    connect();
    Vector<char> bazVector;
    bazVector.append("BAZ", 3);
    EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(bazVector));

    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeBinary, "Bx", 1);
    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeContinuation, "Ax", 1);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeContinuation, "Zx", 1);
}

TEST_F(NewWebSocketChannelImplTest, receiveBinaryWithNullBytes)
{
    connect();
    {
        InSequence s;
        {
            Vector<char> v;
            v.append("\0AR", 3);
            EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));
        }
        {
            Vector<char> v;
            v.append("B\0Z", 3);
            EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));
        }
        {
            Vector<char> v;
            v.append("QU\0", 3);
            EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));
        }
        {
            Vector<char> v;
            v.append("\0\0\0", 3);
            EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));
        }
    }

    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "\0AR", 3);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "B\0Z", 3);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "QU\0", 3);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "\0\0\0", 3);
}

TEST_F(NewWebSocketChannelImplTest, receiveBinaryNonLatin1UTF8)
{
    connect();
    Vector<char> v;
    v.append("\xe7\x8b\x90\xe0\xa4\x94", 6);
    EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));

    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "\xe7\x8b\x90\xe0\xa4\x94", 6);
}

TEST_F(NewWebSocketChannelImplTest, receiveBinaryNonLatin1UTF8Continuation)
{
    connect();
    Vector<char> v;
    v.append("\xe7\x8b\x90\xe0\xa4\x94", 6);
    EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));

    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeBinary, "\xe7\x8b", 2);
    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeContinuation, "\x90\xe0", 2);
    handleClient()->didReceiveData(handle(), false, blink::WebSocketHandle::MessageTypeContinuation, "\xa4", 1);
    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeContinuation, "\x94", 1);
}

TEST_F(NewWebSocketChannelImplTest, receiveBinaryNonUTF8)
{
    connect();
    Vector<char> v;
    v.append("\x80\xff", 2);
    EXPECT_CALL(*channelClient(), didReceiveBinaryDataMock(v));

    handleClient()->didReceiveData(handle(), true, blink::WebSocketHandle::MessageTypeBinary, "\x80\xff", 2);
}

TEST_F(NewWebSocketChannelImplTest, closeFromBrowser)
{
    connect();
    Checkpoint checkpoint;
    {
        InSequence s;

        EXPECT_CALL(*channelClient(), didStartClosingHandshake());
        EXPECT_CALL(checkpoint, Call(1));

        EXPECT_CALL(*handle(), close(WebSocketChannel::CloseEventCodeNormalClosure, blink::WebString("close reason")));
        EXPECT_CALL(checkpoint, Call(2));

        EXPECT_CALL(*channelClient(), didClose(WebSocketChannelClient::ClosingHandshakeComplete, WebSocketChannel::CloseEventCodeNormalClosure, String("close reason")));
        EXPECT_CALL(checkpoint, Call(3));
    }

    handleClient()->didStartClosingHandshake(handle());
    checkpoint.Call(1);

    channel()->close(WebSocketChannel::CloseEventCodeNormalClosure, String("close reason"));
    checkpoint.Call(2);

    handleClient()->didClose(handle(), true, WebSocketChannel::CloseEventCodeNormalClosure, String("close reason"));
    checkpoint.Call(3);

    channel()->disconnect();
}

TEST_F(NewWebSocketChannelImplTest, closeFromWebSocket)
{
    connect();
    Checkpoint checkpoint;
    {
        InSequence s;

        EXPECT_CALL(*handle(), close(WebSocketChannel::CloseEventCodeNormalClosure, blink::WebString("close reason")));
        EXPECT_CALL(checkpoint, Call(1));

        EXPECT_CALL(*channelClient(), didClose(WebSocketChannelClient::ClosingHandshakeComplete, WebSocketChannel::CloseEventCodeNormalClosure, String("close reason")));
        EXPECT_CALL(checkpoint, Call(2));
    }

    channel()->close(WebSocketChannel::CloseEventCodeNormalClosure, String("close reason"));
    checkpoint.Call(1);

    handleClient()->didClose(handle(), true, WebSocketChannel::CloseEventCodeNormalClosure, String("close reason"));
    checkpoint.Call(2);

    channel()->disconnect();
}

TEST_F(NewWebSocketChannelImplTest, failFromBrowser)
{
    connect();
    {
        InSequence s;

        EXPECT_CALL(*channelClient(), didReceiveMessageError());
        EXPECT_CALL(*channelClient(), didClose(WebSocketChannelClient::ClosingHandshakeIncomplete, WebSocketChannel::CloseEventCodeAbnormalClosure, String()));
    }

    handleClient()->didFail(handle(), "fail message");
}

TEST_F(NewWebSocketChannelImplTest, failFromWebSocket)
{
    connect();
    {
        InSequence s;

        EXPECT_CALL(*channelClient(), didReceiveMessageError());
        EXPECT_CALL(*channelClient(), didClose(WebSocketChannelClient::ClosingHandshakeIncomplete, WebSocketChannel::CloseEventCodeAbnormalClosure, String()));
    }

    channel()->fail("fail message from WebSocket", ErrorMessageLevel, "sourceURL", 1234);
}

} // namespace

} // namespace WebCore