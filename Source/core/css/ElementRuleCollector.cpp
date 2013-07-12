/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 */

#include "config.h"
#include "core/css/ElementRuleCollector.h"

#include "core/css/CSSRuleList.h"
#include "core/css/CSSSelector.h"
#include "core/css/SelectorCheckerFastPath.h"
#include "core/css/SiblingTraversalStrategies.h"
#include "core/css/StylePropertySet.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/rendering/RenderRegion.h"

namespace WebCore {

ElementRuleCollector::ElementRuleCollector(const ElementResolveContext& context,
    const SelectorFilter& filter, RenderStyle* style, InspectorCSSOMWrappers& inspectorWrappers)
    : m_context(context)
    , m_selectorFilter(filter)
    , m_inspectorCSSOMWrappers(inspectorWrappers)
    , m_style(style)
    , m_regionForStyling(0)
    , m_pseudoStyleRequest(NOPSEUDO)
    , m_mode(SelectorChecker::ResolvingStyle)
    , m_behaviorAtBoundary(SelectorChecker::DoesNotCrossBoundary)
    , m_canUseFastReject(m_selectorFilter.parentStackIsConsistent(context.parentNode()))
    , m_sameOriginOnly(false)
    , m_matchingUARules(false)
{ }

MatchResult& ElementRuleCollector::matchedResult()
{
    return m_result;
}

PassRefPtr<CSSRuleList> ElementRuleCollector::matchedRuleList()
{
    ASSERT(m_mode == SelectorChecker::CollectingRules);
    return m_ruleList.release();
}

inline void ElementRuleCollector::addMatchedRule(const RuleData* rule)
{
    if (!m_matchedRules)
        m_matchedRules = adoptPtr(new Vector<const RuleData*, 32>);
    m_matchedRules->append(rule);
}

void ElementRuleCollector::clearMatchedRules()
{
    if (!m_matchedRules)
        return;
    m_matchedRules->clear();
}

inline StaticCSSRuleList* ElementRuleCollector::ensureRuleList()
{
    if (!m_ruleList)
        m_ruleList = StaticCSSRuleList::create();
    return m_ruleList.get();
}

void ElementRuleCollector::addElementStyleProperties(const StylePropertySet* propertySet, bool isCacheable)
{
    if (!propertySet)
        return;
    m_result.ranges.lastAuthorRule = m_result.matchedProperties.size();
    if (m_result.ranges.firstAuthorRule == -1)
        m_result.ranges.firstAuthorRule = m_result.ranges.lastAuthorRule;
    m_result.addMatchedProperties(propertySet);
    if (!isCacheable)
        m_result.isCacheable = false;
}

void ElementRuleCollector::collectMatchingRules(const MatchRequest& matchRequest, RuleRange& ruleRange)
{
    ASSERT(matchRequest.ruleSet);
    ASSERT(m_context.element());

    Element* element = m_context.element();
    const AtomicString& pseudoId = element->shadowPseudoId();
    if (!pseudoId.isEmpty()) {
        ASSERT(element->isStyledElement());
        collectMatchingRulesForList(matchRequest.ruleSet->shadowPseudoElementRules(pseudoId.impl()), matchRequest, ruleRange);
    }

    if (element->isWebVTTElement())
        collectMatchingRulesForList(matchRequest.ruleSet->cuePseudoRules(), matchRequest, ruleRange);
    // Check whether other types of rules are applicable in the current tree scope. Criteria for this:
    // a) it's a UA rule
    // b) the tree scope allows author rules
    // c) the rules comes from a scoped style sheet within the same tree scope
    TreeScope* treeScope = element->treeScope();
    if (!m_matchingUARules
        && !treeScope->applyAuthorStyles()
        && (!matchRequest.scope || matchRequest.scope->treeScope() != treeScope)
        && (m_behaviorAtBoundary & SelectorChecker::BoundaryBehaviorMask) == SelectorChecker::DoesNotCrossBoundary)
        return;

    // We need to collect the rules for id, class, tag, and everything else into a buffer and
    // then sort the buffer.
    if (element->hasID())
        collectMatchingRulesForList(matchRequest.ruleSet->idRules(element->idForStyleResolution().impl()), matchRequest, ruleRange);
    if (element->isStyledElement() && element->hasClass()) {
        for (size_t i = 0; i < element->classNames().size(); ++i)
            collectMatchingRulesForList(matchRequest.ruleSet->classRules(element->classNames()[i].impl()), matchRequest, ruleRange);
    }

    if (element->isLink())
        collectMatchingRulesForList(matchRequest.ruleSet->linkPseudoClassRules(), matchRequest, ruleRange);
    if (SelectorChecker::matchesFocusPseudoClass(element))
        collectMatchingRulesForList(matchRequest.ruleSet->focusPseudoClassRules(), matchRequest, ruleRange);
    collectMatchingRulesForList(matchRequest.ruleSet->tagRules(element->localName().impl()), matchRequest, ruleRange);
    collectMatchingRulesForList(matchRequest.ruleSet->universalRules(), matchRequest, ruleRange);
}

void ElementRuleCollector::collectMatchingRulesForRegion(const MatchRequest& matchRequest, RuleRange& ruleRange)
{
    if (!m_regionForStyling)
        return;

    unsigned size = matchRequest.ruleSet->m_regionSelectorsAndRuleSets.size();
    for (unsigned i = 0; i < size; ++i) {
        const CSSSelector* regionSelector = matchRequest.ruleSet->m_regionSelectorsAndRuleSets.at(i).selector;
        if (checkRegionSelector(regionSelector, toElement(m_regionForStyling->node()))) {
            RuleSet* regionRules = matchRequest.ruleSet->m_regionSelectorsAndRuleSets.at(i).ruleSet.get();
            ASSERT(regionRules);
            collectMatchingRules(MatchRequest(regionRules, matchRequest.includeEmptyRules, matchRequest.scope), ruleRange);
        }
    }
}

void ElementRuleCollector::sortAndTransferMatchedRules()
{
    if (!m_matchedRules || m_matchedRules->isEmpty())
        return;

    sortMatchedRules();

    Vector<const RuleData*, 32>& matchedRules = *m_matchedRules;
    if (m_mode == SelectorChecker::CollectingRules) {
        for (unsigned i = 0; i < matchedRules.size(); ++i)
            ensureRuleList()->rules().append(matchedRules[i]->rule()->createCSSOMWrapper());
        return;
    }

    // Now transfer the set of matched rules over to our list of declarations.
    for (unsigned i = 0; i < matchedRules.size(); i++) {
        // FIXME: Matching should not modify the style directly.
        if (m_style && matchedRules[i]->containsUncommonAttributeSelector())
            m_style->setUnique();
        m_result.addMatchedProperties(matchedRules[i]->rule()->properties(), matchedRules[i]->rule(), matchedRules[i]->linkMatchType(), matchedRules[i]->propertyWhitelistType(m_matchingUARules));
    }
}

inline bool ElementRuleCollector::ruleMatches(const RuleData& ruleData, const ContainerNode* scope, PseudoId& dynamicPseudo)
{
    if (ruleData.hasFastCheckableSelector()) {
        // We know this selector does not include any pseudo elements.
        if (m_pseudoStyleRequest.pseudoId != NOPSEUDO)
            return false;
        // We know a sufficiently simple single part selector matches simply because we found it from the rule hash.
        // This is limited to HTML only so we don't need to check the namespace.
        if (ruleData.hasRightmostSelectorMatchingHTMLBasedOnRuleHash() && m_context.element()->isHTMLElement()) {
            if (!ruleData.hasMultipartSelector())
                return true;
        }
        if (ruleData.selector()->m_match == CSSSelector::Tag && !SelectorChecker::tagMatches(m_context.element(), ruleData.selector()->tagQName()))
            return false;
        SelectorCheckerFastPath selectorCheckerFastPath(ruleData.selector(), m_context.element());
        if (!selectorCheckerFastPath.matchesRightmostAttributeSelector())
            return false;

        return selectorCheckerFastPath.matches();
    }

    // Slow path.
    SelectorChecker selectorChecker(document(), m_mode);
    SelectorChecker::SelectorCheckingContext context(ruleData.selector(), m_context.element(), SelectorChecker::VisitedMatchEnabled);
    context.elementStyle = m_style.get();
    context.scope = scope;
    context.pseudoId = m_pseudoStyleRequest.pseudoId;
    context.scrollbar = m_pseudoStyleRequest.scrollbar;
    context.scrollbarPart = m_pseudoStyleRequest.scrollbarPart;
    context.behaviorAtBoundary = m_behaviorAtBoundary;
    SelectorChecker::Match match = selectorChecker.match(context, dynamicPseudo, DOMSiblingTraversalStrategy());
    if (match != SelectorChecker::SelectorMatches)
        return false;
    if (m_pseudoStyleRequest.pseudoId != NOPSEUDO && m_pseudoStyleRequest.pseudoId != dynamicPseudo)
        return false;
    return true;
}

void ElementRuleCollector::collectRuleIfMatches(const RuleData& ruleData, const MatchRequest& matchRequest, RuleRange& ruleRange)
{
    if (m_canUseFastReject && m_selectorFilter.fastRejectSelector(ruleData.selector()))
        return;

    StyleRule* rule = ruleData.rule();
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willMatchRule(document(), rule, m_inspectorCSSOMWrappers, document()->styleSheetCollection());
    PseudoId dynamicPseudo = NOPSEUDO;
    if (ruleMatches(ruleData, matchRequest.scope, dynamicPseudo)) {
        // If the rule has no properties to apply, then ignore it in the non-debug mode.
        const StylePropertySet* properties = rule->properties();
        if (!properties || (properties->isEmpty() && !matchRequest.includeEmptyRules)) {
            InspectorInstrumentation::didMatchRule(cookie, false);
            return;
        }
        // FIXME: Exposing the non-standard getMatchedCSSRules API to web is the only reason this is needed.
        if (m_sameOriginOnly && !ruleData.hasDocumentSecurityOrigin()) {
            InspectorInstrumentation::didMatchRule(cookie, false);
            return;
        }
        // If we're matching normal rules, set a pseudo bit if
        // we really just matched a pseudo-element.
        if (dynamicPseudo != NOPSEUDO && m_pseudoStyleRequest.pseudoId == NOPSEUDO) {
            if (m_mode == SelectorChecker::CollectingRules) {
                InspectorInstrumentation::didMatchRule(cookie, false);
                return;
            }
            // FIXME: Matching should not modify the style directly.
            if (dynamicPseudo < FIRST_INTERNAL_PSEUDOID)
                m_style->setHasPseudoStyle(dynamicPseudo);
        } else {
            // Update our first/last rule indices in the matched rules array.
            ++ruleRange.lastRuleIndex;
            if (ruleRange.firstRuleIndex == -1)
                ruleRange.firstRuleIndex = ruleRange.lastRuleIndex;

            // Add this rule to our list of matched rules.
            addMatchedRule(&ruleData);
            InspectorInstrumentation::didMatchRule(cookie, true);
            return;
        }
    }
    InspectorInstrumentation::didMatchRule(cookie, false);
}

void ElementRuleCollector::collectMatchingRulesForList(const RuleData* rules, const MatchRequest& matchRequest, RuleRange& ruleRange)
{
    if (!rules)
        return;
    while (!rules->isLastInArray())
        collectRuleIfMatches(*rules++, matchRequest, ruleRange);
    collectRuleIfMatches(*rules, matchRequest, ruleRange);
}

void ElementRuleCollector::collectMatchingRulesForList(const Vector<RuleData>* rules, const MatchRequest& matchRequest, RuleRange& ruleRange)
{
    if (!rules)
        return;
    unsigned size = rules->size();
    for (unsigned i = 0; i < size; ++i)
        collectRuleIfMatches(rules->at(i), matchRequest, ruleRange);
}

static inline bool compareRules(const RuleData* r1, const RuleData* r2)
{
    unsigned specificity1 = r1->specificity();
    unsigned specificity2 = r2->specificity();
    return (specificity1 == specificity2) ? r1->position() < r2->position() : specificity1 < specificity2;
}

void ElementRuleCollector::sortMatchedRules()
{
    ASSERT(m_matchedRules);
    std::sort(m_matchedRules->begin(), m_matchedRules->end(), compareRules);
}

bool ElementRuleCollector::hasAnyMatchingRules(RuleSet* ruleSet)
{
    clearMatchedRules();

    m_mode = SelectorChecker::SharingRules;
    // To check whether a given RuleSet has any rule matching a given element,
    // should not see the element's treescope. Because RuleSet has no
    // information about "scope".
    m_behaviorAtBoundary = SelectorChecker::StaysWithinTreeScope;
    int firstRuleIndex = -1, lastRuleIndex = -1;
    RuleRange ruleRange(firstRuleIndex, lastRuleIndex);
    collectMatchingRules(MatchRequest(ruleSet), ruleRange);

    return m_matchedRules && !m_matchedRules->isEmpty();
}

} // namespace WebCore
