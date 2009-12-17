/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebDocument.h"

#include "Document.h"
#include "Element.h"
#include "HTMLAllCollection.h"
#include "HTMLBodyElement.h"
#include "HTMLCollection.h"
#include "HTMLElement.h"
#include "HTMLHeadElement.h"

#include "WebElement.h"
#include "WebFrameImpl.h"
#include "WebNodeCollection.h"
#include "WebURL.h"

#include <wtf/PassRefPtr.h>

using namespace WebCore;

namespace WebKit {

WebDocument::WebDocument(const PassRefPtr<Document>& elem)
    : WebNode(elem.releaseRef())
{
}

WebDocument& WebDocument::operator=(const PassRefPtr<Document>& elem)
{
    WebNode::assign(elem.releaseRef());
    return *this;
}

WebDocument::operator PassRefPtr<Document>() const
{
    return PassRefPtr<Document>(static_cast<Document*>(m_private));
}

WebFrame* WebDocument::frame() const
{
    return WebFrameImpl::fromFrame(constUnwrap<Document>()->frame());
}

bool WebDocument::isHTMLDocument() const
{  
    return constUnwrap<Document>()->isHTMLDocument();
}

WebURL WebDocument::baseURL() const
{
    return constUnwrap<Document>()->baseURL();
}

WebElement WebDocument::body() const
{
    return WebElement(constUnwrap<Document>()->body());
}

WebElement WebDocument::head()
{
    return WebElement(unwrap<Document>()->head());
}

WebNodeCollection WebDocument::all()
{
    return WebNodeCollection(unwrap<Document>()->all());
}

WebURL WebDocument::completeURL(const WebString& partialURL) const
{
    return constUnwrap<Document>()->completeURL(partialURL);
}

} // namespace WebKit
