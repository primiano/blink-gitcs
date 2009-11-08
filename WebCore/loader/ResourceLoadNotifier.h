/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ResourceLoadNotifier_h
#define ResourceLoadNotifier_h

#include <wtf/Noncopyable.h>

namespace WebCore {

class AuthenticationChallenge;
class DocumentLoader;
class Frame;
class ResourceError;
class ResourceLoader;
class ResourceResponse;
class ScriptString;
class ResourceRequest;

class ResourceLoadNotifier : public Noncopyable {
public:
    ResourceLoadNotifier(Frame*);

    void didReceiveAuthenticationChallenge(ResourceLoader*, const AuthenticationChallenge&);
    void didCancelAuthenticationChallenge(ResourceLoader*, const AuthenticationChallenge&);

    void willSendRequest(ResourceLoader*, ResourceRequest&, const ResourceResponse& redirectResponse);
    void didReceiveResponse(ResourceLoader*, const ResourceResponse&);
    void didReceiveData(ResourceLoader*, const char*, int, int lengthReceived);
    void didFinishLoad(ResourceLoader*);
    void didFailToLoad(ResourceLoader*, const ResourceError&);
    void didLoadResourceByXMLHttpRequest(unsigned long identifier, const ScriptString& sourceString);

    void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);
    void dispatchWillSendRequest(DocumentLoader*, unsigned long identifier, ResourceRequest&, const ResourceResponse& redirectResponse);
    void dispatchDidReceiveResponse(DocumentLoader*, unsigned long identifier, const ResourceResponse&);
    void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long identifier, int length);
    void dispatchDidFinishLoading(DocumentLoader*, unsigned long identifier);

    void sendRemainingDelegateMessages(DocumentLoader*, unsigned long identifier, const ResourceResponse&, int length, const ResourceError&);

private:
    Frame* m_frame;
};

} // namespace WebCore

#endif // ResourceLoadNotifier_h
