/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSHTMLElement.h"

#include "Document.h"
#include "HTMLElement.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace KJS;
using namespace HTMLNames;

void JSHTMLElement::pushEventHandlerScope(ExecState* exec, ScopeChain& scope) const
{
    HTMLElement* element = static_cast<HTMLElement*>(impl());

    // The document is put on first, fall back to searching it only after the element and form.
    scope.push(static_cast<JSObject*>(toJS(exec, element->ownerDocument())));

    // The form is next, searched before the document, but after the element itself.

    // First try to obtain the form from the element itself.  We do this to deal with
    // the malformed case where <form>s aren't in our parent chain (e.g., when they were inside 
    // <table> or <tbody>.
    HTMLFormElement* form = element->formForEventHandlerScope();
    if (form)
        scope.push(static_cast<JSObject*>(toJS(exec, form)));
    else {
        Node* form = element->parentNode();
        while (form && !form->hasTagName(formTag))
            form = form->parentNode();

        if (form)
            scope.push(static_cast<JSObject*>(toJS(exec, form)));
    }

    // The element is on top, searched first.
    scope.push(static_cast<JSObject*>(toJS(exec, element)));
}

} // namespace WebCore
