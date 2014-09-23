/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef SelectRuleFeatureSet_h
#define SelectRuleFeatureSet_h

#include "core/css/RuleFeature.h"
#include "core/dom/Element.h"

namespace blink {

class SpaceSplitString;

class SelectRuleFeatureSet {
    DISALLOW_ALLOCATION();
public:
    void add(const SelectRuleFeatureSet&);
    void clear();
    void collectFeaturesFromSelector(const CSSSelector&);

    bool hasSelectorForId(const AtomicString&) const;
    bool hasSelectorForClass(const AtomicString&) const;
    bool hasSelectorForAttribute(const AtomicString&) const;
    bool hasSelectorForPseudoType(CSSSelector::PseudoType) const;

    bool hasSelectorForChecked() const { return hasSelectorForPseudoType(CSSSelector::PseudoChecked); }
    bool hasSelectorForEnabled() const { return hasSelectorForPseudoType(CSSSelector::PseudoEnabled); }
    bool hasSelectorForDisabled() const { return hasSelectorForPseudoType(CSSSelector::PseudoDisabled); }
    bool hasSelectorForIndeterminate() const { return hasSelectorForPseudoType(CSSSelector::PseudoIndeterminate); }
    bool hasSelectorForLink() const { return hasSelectorForPseudoType(CSSSelector::PseudoLink); }
    bool hasSelectorForTarget() const { return hasSelectorForPseudoType(CSSSelector::PseudoTarget); }
    bool hasSelectorForVisited() const { return hasSelectorForPseudoType(CSSSelector::PseudoVisited); }

    bool checkSelectorsForClassChange(const SpaceSplitString& changedClasses) const;
    bool checkSelectorsForClassChange(const SpaceSplitString& oldClasses, const SpaceSplitString& newClasses) const;

    void trace(Visitor* visitor) { visitor->trace(m_cssRuleFeatureSet); }

private:
    RuleFeatureSet m_cssRuleFeatureSet;
};

inline bool SelectRuleFeatureSet::hasSelectorForId(const AtomicString& idValue) const
{
    ASSERT(!idValue.isEmpty());
    return m_cssRuleFeatureSet.hasSelectorForId(idValue);
}

inline bool SelectRuleFeatureSet::hasSelectorForClass(const AtomicString& classValue) const
{
    ASSERT(!classValue.isEmpty());
    return m_cssRuleFeatureSet.hasSelectorForClass(classValue);
}

inline bool SelectRuleFeatureSet::hasSelectorForAttribute(const AtomicString& attributeName) const
{
    ASSERT(!attributeName.isEmpty());
    return m_cssRuleFeatureSet.hasSelectorForAttribute(attributeName);
}

inline bool SelectRuleFeatureSet::hasSelectorForPseudoType(CSSSelector::PseudoType pseudo) const
{
    return m_cssRuleFeatureSet.hasSelectorForPseudoType(pseudo);
}

}

#endif
