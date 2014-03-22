
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "core/css/invalidation/StyleInvalidator.h"

#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ElementTraversal.h"
#include "core/dom/shadow/ElementShadow.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/rendering/RenderObject.h"

namespace WebCore {

void StyleInvalidator::invalidate()
{
    if (Element* documentElement = m_document.documentElement())
        invalidate(*documentElement);
    m_document.clearChildNeedsStyleInvalidation();
    m_document.clearNeedsStyleInvalidation();
    m_pendingInvalidationMap.clear();
}

StyleInvalidator::StyleInvalidator(Document& document)
    : m_document(document)
    , m_pendingInvalidationMap(document.styleResolver()->ruleFeatureSet().pendingInvalidationMap())
{ }

void StyleInvalidator::RecursionData::pushInvalidationSet(const DescendantInvalidationSet& invalidationSet)
{
    invalidationSet.getClasses(m_invalidationClasses);
    invalidationSet.getAttributes(m_invalidationAttributes);
    m_foundInvalidationSet = true;
}

bool StyleInvalidator::RecursionData::matchesCurrentInvalidationSets(Element& element)
{
    if (element.hasClass()) {
        const SpaceSplitString& classNames = element.classNames();
        for (Vector<AtomicString>::const_iterator it = m_invalidationClasses.begin(); it != m_invalidationClasses.end(); ++it) {
            if (classNames.contains(*it))
                return true;
        }
    }
    if (element.hasAttributes()) {
        for (Vector<AtomicString>::const_iterator it = m_invalidationAttributes.begin(); it != m_invalidationAttributes.end(); ++it) {
            if (element.hasAttribute(*it))
                return true;
        }
    }

    return false;
}

bool StyleInvalidator::checkInvalidationSetsAgainstElement(Element& element)
{
    bool thisElementNeedsStyleRecalc = false;
    if (element.needsStyleInvalidation()) {
        if (RuleFeatureSet::InvalidationList* invalidationList = m_pendingInvalidationMap.get(&element)) {
            // FIXME: it's really only necessary to clone the render style for this element, not full style recalc.
            thisElementNeedsStyleRecalc = true;
            for (RuleFeatureSet::InvalidationList::const_iterator it = invalidationList->begin(); it != invalidationList->end(); ++it) {
                m_recursionData.pushInvalidationSet(**it);
                if ((*it)->wholeSubtreeInvalid()) {
                    element.setNeedsStyleRecalc(SubtreeStyleChange);
                    // Even though we have set needsStyleRecalc on the whole subtree, we need to keep walking over the subtree
                    // in order to clear the invalidation dirty bits on all elements.
                    // FIXME: we can optimize this by having a dedicated function that just traverses the tree and removes the dirty bits,
                    // without checking classes etc.
                    break;
                }
            }
        }
    }
    if (!thisElementNeedsStyleRecalc)
        thisElementNeedsStyleRecalc = m_recursionData.matchesCurrentInvalidationSets(element);
    return thisElementNeedsStyleRecalc;
}

bool StyleInvalidator::invalidateChildren(Element& element)
{
    bool someChildrenNeedStyleRecalc = false;
    for (ShadowRoot* root = element.youngestShadowRoot(); root; root = root->olderShadowRoot()) {
        for (Element* child = ElementTraversal::firstWithin(*root); child; child = ElementTraversal::nextSibling(*child)) {
            bool childRecalced = invalidate(*child);
            someChildrenNeedStyleRecalc = someChildrenNeedStyleRecalc || childRecalced;
        }
        root->clearChildNeedsStyleInvalidation();
        root->clearNeedsStyleInvalidation();
    }
    for (Element* child = ElementTraversal::firstWithin(element); child; child = ElementTraversal::nextSibling(*child)) {
        bool childRecalced = invalidate(*child);
        someChildrenNeedStyleRecalc = someChildrenNeedStyleRecalc || childRecalced;
    }
    return someChildrenNeedStyleRecalc;
}

bool StyleInvalidator::invalidate(Element& element)
{
    RecursionCheckpoint checkpoint(&m_recursionData);

    bool thisElementNeedsStyleRecalc = checkInvalidationSetsAgainstElement(element);

    bool someChildrenNeedStyleRecalc = false;
    // foundInvalidationSet() will be true if we are in a subtree of a node with a DescendantInvalidationSet on it.
    // We need to check all nodes in the subtree of such a node.
    if (m_recursionData.foundInvalidationSet() || element.childNeedsStyleInvalidation())
        someChildrenNeedStyleRecalc = invalidateChildren(element);

    if (thisElementNeedsStyleRecalc) {
        element.setNeedsStyleRecalc(LocalStyleChange);
    } else if (m_recursionData.foundInvalidationSet() && someChildrenNeedStyleRecalc) {
        // Clone the RenderStyle in order to preserve correct style sharing, if possible. Otherwise recalc style.
        if (RenderObject* renderer = element.renderer()) {
            ASSERT(renderer->style());
            renderer->setStyleInternal(RenderStyle::clone(renderer->style()));
        } else {
            element.setNeedsStyleRecalc(LocalStyleChange);
        }
    }

    element.clearChildNeedsStyleInvalidation();
    element.clearNeedsStyleInvalidation();

    return thisElementNeedsStyleRecalc;
}

} // namespace WebCore
