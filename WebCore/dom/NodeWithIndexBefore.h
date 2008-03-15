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

#ifndef NodeWithIndexBefore_h
#define NodeWithIndexBefore_h

#include "ContainerNode.h"

namespace WebCore {

// Same as NodeWithIndex, but treats a node pointer of 0 as meaning
// "after the last node in the container".
class NodeWithIndexBefore {
public:
    NodeWithIndexBefore(ContainerNode* parent, Node* node)
        : m_parent(parent)
        , m_node(node)
        , m_haveIndex(false)
    {
        ASSERT(parent);
        ASSERT(!node || node->parentNode() == parent);
    }

    ContainerNode* parent() const { return m_parent; }

    int indexBefore() const
    {
        if (!m_haveIndex) {
            m_indexBefore = m_node ? m_node->nodeIndex() : m_parent->childNodeCount();
            m_haveIndex = true;
        }
        ASSERT(m_indexBefore == static_cast<int>(m_node ? m_node->nodeIndex() : m_parent->childNodeCount()));
        return m_indexBefore;
    }

private:
    ContainerNode* m_parent;
    Node* m_node;
    mutable bool m_haveIndex;
    mutable int m_indexBefore;
};

}

#endif
