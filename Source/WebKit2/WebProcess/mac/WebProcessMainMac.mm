/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebProcessMain.h"

#import "CommandLine.h"
#import "ChildProcessMain.h"
#import "EnvironmentUtilities.h"
#import "EnvironmentVariables.h"
#import "StringUtilities.h"
#import "WebProcess.h"
#import <mach/mach_error.h>
#import <servers/bootstrap.h>
#import <spawn.h>
#import <stdio.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>

extern "C" kern_return_t bootstrap_register2(mach_port_t, name_t, mach_port_t, uint64_t);

namespace WebKit {

class WebProcessMainDelegate : public ChildProcessMainDelegate {
public:
    WebProcessMainDelegate(const CommandLine& commandLine)
        : ChildProcessMainDelegate(commandLine)
    {
    }

    virtual void doPreInitializationWork()
    {
        // Remove the WebProcess shim from the DYLD_INSERT_LIBRARIES environment variable so any processes spawned by
        // the WebProcess don't try to insert the shim and crash.
        EnvironmentUtilities::stripValuesEndingWithString("DYLD_INSERT_LIBRARIES", "/SecItemShim.dylib");
    }

    virtual bool getConnectionIdentifier(CoreIPC::Connection::Identifier& identifier)
    {
        String serviceName = m_commandLine["servicename"];
        String clientExecutable = m_commandLine["client-executable"];
        if (serviceName.isEmpty() && clientExecutable.isEmpty())
            return false;

        mach_port_t serverPort;
        if (clientExecutable.isEmpty()) {
            kern_return_t kr = bootstrap_look_up(bootstrap_port, serviceName.utf8().data(), &serverPort);
            if (kr) {
                WTFLogAlways("bootstrap_look_up result: %s (%x)\n", mach_error_string(kr), kr);
                return false;
            }
        } else {
            mach_port_name_t publishedService;
            mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &publishedService);
            mach_port_insert_right(mach_task_self(), publishedService, publishedService, MACH_MSG_TYPE_MAKE_SEND);
            // Make it possible to look up.
            serviceName = String::format("com.apple.WebKit.WebProcess-%d", getpid());
            if (kern_return_t kr = bootstrap_register2(bootstrap_port, const_cast<char*>(serviceName.utf8().data()), publishedService, 0)) {
                WTFLogAlways("Failed to register service name \"%s\". %s (%x)\n", serviceName.utf8().data(), mach_error_string(kr), kr);
                return false;
            }

            CString command = clientExecutable.utf8();
            const char* args[] = { command.data(), 0 };

            EnvironmentVariables environmentVariables;
            environmentVariables.set(EnvironmentVariables::preexistingProcessServiceNameKey(), serviceName.utf8().data());
            environmentVariables.set(EnvironmentVariables::preexistingProcessTypeKey(), m_commandLine["type"].utf8().data());

            posix_spawn_file_actions_t fileActions;
            posix_spawn_file_actions_init(&fileActions);
            posix_spawn_file_actions_addinherit_np(&fileActions, STDIN_FILENO);
            posix_spawn_file_actions_addinherit_np(&fileActions, STDOUT_FILENO);
            posix_spawn_file_actions_addinherit_np(&fileActions, STDERR_FILENO);

            posix_spawnattr_t attributes;
            posix_spawnattr_init(&attributes);
            posix_spawnattr_setflags(&attributes, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_SETPGROUP);

            int spawnResult = posix_spawn(0, command.data(), &fileActions, &attributes, const_cast<char**>(args), environmentVariables.environmentPointer());

            posix_spawnattr_destroy(&attributes);
            posix_spawn_file_actions_destroy(&fileActions);

            if (spawnResult)
                return false;

            mach_msg_empty_rcv_t message;
            if (kern_return_t kr = mach_msg(&message.header, MACH_RCV_MSG, 0, sizeof(message), publishedService, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL)) {
                WTFLogAlways("Failed to receive port from the UI process. %s (%x)\n", mach_error_string(kr), kr);
                return false;
            }

            mach_port_mod_refs(mach_task_self(), publishedService, MACH_PORT_RIGHT_RECEIVE, -1);
            serverPort = message.header.msgh_remote_port;
            mach_port_type_t portType;
            kern_return_t kr = mach_port_type(mach_task_self(), serverPort, &portType);
            if (kr || !(portType & MACH_PORT_TYPE_SEND)) {
                WTFLogAlways("Failed to obtain send right for port received from the UI process.\n");
                return false;
            }
        }

        identifier = serverPort;
        return true;
    }

    virtual bool getClientIdentifier(String& clientIdentifier)
    {
        String clientExecutable = m_commandLine["client-executable"];

        if (clientExecutable.isEmpty())
            clientIdentifier = m_commandLine["client-identifier"];
        else {
            RetainPtr<NSURL> clientExecutableURL = adoptNS([[NSURL alloc] initFileURLWithPath:nsStringFromWebCoreString(clientExecutable)]);
            RetainPtr<CFURLRef> clientBundleURL = adoptCF(WKCopyBundleURLForExecutableURL((CFURLRef)clientExecutableURL.get()));
            RetainPtr<NSBundle> clientBundle = adoptNS([[NSBundle alloc] initWithURL:(NSURL *)clientBundleURL.get()]);
            clientIdentifier = [clientBundle.get() bundleIdentifier];
        }

        if (clientIdentifier.isEmpty())
            return false;
        return true;
    }
};

int WebProcessMain(const CommandLine& commandLine)
{
    return ChildProcessMain<WebProcess, WebProcessMainDelegate>(commandLine);
}

} // namespace WebKit

