/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#include "dom_edititerator.h"

#include "dom_nodeimpl.h"

namespace DOM {

DOMPosition EditIterator::peekPrevious() const
{
    DOMPosition pos = m_current;
    
    if (pos.isEmpty())
        return pos;
    
    if (pos.offset() <= 0) {
        NodeImpl *prevNode = pos.node()->previousEditable();
        if (prevNode)
            pos = DOMPosition(prevNode, prevNode->maxOffset());
    }
    else {
        pos = DOMPosition(pos.node(), pos.offset() - 1);
    }
    
    return pos;
}

DOMPosition EditIterator::peekNext() const
{
    DOMPosition pos = m_current;
    
    if (pos.isEmpty())
        return pos;
    
    if (pos.offset() >= pos.node()->maxOffset()) {
        NodeImpl *nextNode = pos.node()->nextEditable();
        if (nextNode)
            pos = DOMPosition(nextNode, 0);
    }
    else {
        pos = DOMPosition(pos.node(), pos.offset() + 1);
    }
    
    return pos;
}

bool EditIterator::atStart() const
{
    if (m_current.isEmpty())
        return true;

    return m_current.offset() == 0 && 
        m_current.node()->previousEditable() == 0;
}

bool EditIterator::atEnd() const
{
    if (m_current.isEmpty())
        return true;

    return m_current.offset() == m_current.node()->maxOffset() && 
        m_current.node()->nextEditable() == 0;
}

} // namespace DOM
