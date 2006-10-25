/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
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

#import "config.h"
#import "WebNetscapePlugInStreamLoader.h"

#import "FrameLoader.h"
#import <wtf/PassRefPtr.h>

namespace WebCore {

NetscapePlugInStreamLoader::NetscapePlugInStreamLoader(Frame* frame, id <WebPlugInStreamLoaderDelegate> stream)
    : WebResourceLoader(frame)
    , m_stream(stream)
{
}

NetscapePlugInStreamLoader::~NetscapePlugInStreamLoader()
{
}

PassRefPtr<NetscapePlugInStreamLoader> NetscapePlugInStreamLoader::create(Frame* frame, id <WebPlugInStreamLoaderDelegate> d)
{
    return new NetscapePlugInStreamLoader(frame, d);
}

bool NetscapePlugInStreamLoader::isDone() const
{
    return !m_stream;
}

void NetscapePlugInStreamLoader::releaseResources()
{
    m_stream = nil;
    WebResourceLoader::releaseResources();
}

void NetscapePlugInStreamLoader::didReceiveResponse(NSURLResponse *theResponse)
{
    // Protect self in this delegate method since the additional processing can do
    // anything including possibly getting rid of the last reference to this object.
    // One example of this is Radar 3266216.
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    [m_stream.get() startStreamWithResponse:theResponse];
    
    // Don't continue if the stream is cancelled in startStreamWithResponse or didReceiveResponse.
    if (!m_stream)
        return;
    WebResourceLoader::didReceiveResponse(theResponse);
    if (!m_stream)
        return;
    if ([theResponse isKindOfClass:[NSHTTPURLResponse class]] &&
        ([(NSHTTPURLResponse *)theResponse statusCode] >= 400 || [(NSHTTPURLResponse *)theResponse statusCode] < 100)) {
        NSError *error = frameLoader()->fileDoesNotExistError(theResponse);
        [m_stream.get() cancelLoadAndDestroyStreamWithError:error];
    }
}

void NetscapePlugInStreamLoader::didReceiveData(NSData *data, long long lengthReceived, bool allAtOnce)
{
    // Protect self in this delegate method since the additional processing can do
    // anything including possibly getting rid of the last reference to this object.
    // One example of this is Radar 3266216.
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    [m_stream.get() receivedData:data];
    WebResourceLoader::didReceiveData(data, lengthReceived, allAtOnce);
}

void NetscapePlugInStreamLoader::didFinishLoading()
{
    // Calling removePlugInStreamLoader will likely result in a call to deref, so we must protect.
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    frameLoader()->removePlugInStreamLoader(this);
    [m_stream.get() finishedLoadingWithData:resourceData()];
    WebResourceLoader::didFinishLoading();
}

void NetscapePlugInStreamLoader::didFail(NSError *error)
{
    // Protect self in this delegate method since the additional processing can do
    // anything including possibly getting rid of the last reference to this object.
    // One example of this is Radar 3266216.
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    frameLoader()->removePlugInStreamLoader(this);
    [m_stream.get() destroyStreamWithError:error];
    WebResourceLoader::didFail(error);
}

void NetscapePlugInStreamLoader::didCancel(NSError *error)
{
    // Calling removePlugInStreamLoader will likely result in a call to deref, so we must protect.
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    frameLoader()->removePlugInStreamLoader(this);
    [m_stream.get() destroyStreamWithError:error];
    WebResourceLoader::didCancel(error);
}

}
