/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *           (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef ContainerNodeAlgorithms_h
#define ContainerNodeAlgorithms_h

#include "core/dom/Document.h"
#include "core/dom/ScriptForbiddenScope.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "wtf/Assertions.h"

namespace WebCore {

class ChildNodeInsertionNotifier {
public:
    explicit ChildNodeInsertionNotifier(ContainerNode& insertionPoint)
        : m_insertionPoint(insertionPoint)
    {
    }

    void notify(Node&);

private:
    void notifyNodeInserted(Node&);

    ContainerNode& m_insertionPoint;
    Vector< RefPtr<Node> > m_postInsertionNotificationTargets;
};

inline void ChildNodeInsertionNotifier::notify(Node& node)
{
    ASSERT(!NoEventDispatchAssertion::isEventDispatchForbidden());

    InspectorInstrumentation::didInsertDOMNode(&node);

    RefPtr<Document> protectDocument(node.document());
    RefPtr<Node> protectNode(node);

    {
        NoEventDispatchAssertion assertNoEventDispatch;
        ScriptForbiddenScope forbidScript;
        notifyNodeInserted(node);
    }

    for (size_t i = 0; i < m_postInsertionNotificationTargets.size(); ++i) {
        Node* targetNode = m_postInsertionNotificationTargets[i].get();
        if (targetNode->inDocument())
            targetNode->didNotifySubtreeInsertionsToDocument();
    }
}

class ChildNodeRemovalNotifier {
public:
    explicit ChildNodeRemovalNotifier(ContainerNode& insertionPoint)
        : m_insertionPoint(insertionPoint)
    {
    }

    void notify(Node&);

private:
    void notifyNodeRemoved(Node&);

    ContainerNode& m_insertionPoint;
};

inline void ChildNodeRemovalNotifier::notify(Node& node)
{
    ScriptForbiddenScope forbidScript;
    NoEventDispatchAssertion assertNoEventDispatch;
    notifyNodeRemoved(node);
}

namespace Private {

    template<class GenericNode, class GenericNodeContainer>
    void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer&);

}

#if !ENABLE(OILPAN)
// Helper functions for TreeShared-derived classes, which have a 'Node' style interface
// This applies to 'ContainerNode' and 'SVGElementInstance'
template<class GenericNode, class GenericNodeContainer>
inline void removeDetachedChildrenInContainer(GenericNodeContainer& container)
{
    // List of nodes to be deleted.
    GenericNode* head = 0;
    GenericNode* tail = 0;

    Private::addChildNodesToDeletionQueue<GenericNode, GenericNodeContainer>(head, tail, container);

    GenericNode* n;
    GenericNode* next;
    while ((n = head) != 0) {
        ASSERT_WITH_SECURITY_IMPLICATION(n->m_deletionHasBegun);

        next = n->nextSibling();
        n->setNextSibling(0);

        head = next;
        if (next == 0)
            tail = 0;

        if (n->hasChildren())
            Private::addChildNodesToDeletionQueue<GenericNode, GenericNodeContainer>(head, tail, static_cast<GenericNodeContainer&>(*n));

        delete n;
    }
}
#endif

template<class GenericNode, class GenericNodeContainer>
inline void appendChildToContainer(GenericNode& child, GenericNodeContainer& container)
{
    child.setParentOrShadowHostNode(&container);

    GenericNode* lastChild = container.lastChild();
    if (lastChild) {
        child.setPreviousSibling(lastChild);
        lastChild->setNextSibling(&child);
    } else {
        container.setFirstChild(&child);
    }

    container.setLastChild(&child);
}

// Helper methods for removeDetachedChildrenInContainer, hidden from WebCore namespace
namespace Private {

    template<class GenericNode, class GenericNodeContainer, bool dispatchRemovalNotification>
    struct NodeRemovalDispatcher {
        static void dispatch(GenericNode&, GenericNodeContainer&)
        {
            // no-op, by default
        }
    };

    template<class GenericNode, class GenericNodeContainer>
    struct NodeRemovalDispatcher<GenericNode, GenericNodeContainer, true> {
        static void dispatch(GenericNode& node, GenericNodeContainer& container)
        {
            container.document().adoptIfNeeded(node);
            if (node.inDocument())
                ChildNodeRemovalNotifier(container).notify(node);
        }
    };

    template<class GenericNode>
    struct ShouldDispatchRemovalNotification {
        static const bool value = false;
    };

    template<>
    struct ShouldDispatchRemovalNotification<Node> {
        static const bool value = true;
    };

    template<class GenericNode, class GenericNodeContainer>
    void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer& container)
    {
        // We have to tell all children that their parent has died.
        GenericNode* next = 0;
        for (GenericNode* n = container.firstChild(); n; n = next) {
            ASSERT_WITH_SECURITY_IMPLICATION(!n->m_deletionHasBegun);

            next = n->nextSibling();
            n->setNextSibling(0);
            n->setParentOrShadowHostNode(0);
            container.setFirstChild(next);
            if (next)
                next->setPreviousSibling(0);

            if (!n->refCount()) {
#if SECURITY_ASSERT_ENABLED
                n->m_deletionHasBegun = true;
#endif
                // Add the node to the list of nodes to be deleted.
                // Reuse the nextSibling pointer for this purpose.
                if (tail)
                    tail->setNextSibling(n);
                else
                    head = n;

                tail = n;
            } else {
                RefPtr<GenericNode> protect(n); // removedFromDocument may remove all references to this node.
                NodeRemovalDispatcher<GenericNode, GenericNodeContainer, ShouldDispatchRemovalNotification<GenericNode>::value>::dispatch(*n, container);
            }
        }

        container.setLastChild(0);
    }

} // namespace Private

} // namespace WebCore

#endif // ContainerNodeAlgorithms_h
