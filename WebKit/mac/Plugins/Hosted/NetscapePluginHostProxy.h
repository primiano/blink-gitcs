/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS)

#ifndef NetscapePluginHostProxy_h
#define NetscapePluginHostProxy_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebKit {
    
class NetscapePluginInstanceProxy;

class NetscapePluginHostProxy : public RefCounted<NetscapePluginHostProxy> {
public:
    static PassRefPtr<NetscapePluginHostProxy> create(mach_port_t pluginHostPort)
    {
        return adoptRef(new NetscapePluginHostProxy(pluginHostPort));
    }
    
    PassRefPtr<NetscapePluginInstanceProxy> instantiatePlugin(NSString *mimeType, NSArray *attributeKeys, NSArray *attributeValues, NSString *userAgent, NSURL *sourceURL);
    
    mach_port_t port() const { return m_pluginHostPort; }

private:
    NetscapePluginHostProxy(mach_port_t pluginHostPort);

    mach_port_t m_pluginHostPort;
};
    
} // namespace WebKit

#endif // NetscapePluginHostProxy_h
#endif // USE(PLUGIN_HOST_PROCESS)
