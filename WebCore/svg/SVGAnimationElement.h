/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>
    Copyright (C) 2008 Apple Inc. All rights reserved.

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef SVGAnimationElement_h
#define SVGAnimationElement_h
#if ENABLE(SVG_ANIMATION)

#include "ElementTimeControl.h"
#include "SMILTime.h"
#include "SVGSMILElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGStringList.h"
#include "SVGTests.h"
#include "UnitBezier.h"

namespace WebCore {
    
    class ConditionEventListener;
    class TimeContainer;

    class SVGAnimationElement : public SVGSMILElement,
                                public SVGTests,
                                public SVGExternalResourcesRequired,
                                public ElementTimeControl
    {
    public:
        SVGAnimationElement(const QualifiedName&, Document*);
        virtual ~SVGAnimationElement();
        
        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void attributeChanged(Attribute*, bool preserveDecls);

        // SVGAnimationElement
        float getStartTime() const;
        float getCurrentTime() const;
        float getSimpleDuration(ExceptionCode&) const;
        
        // ElementTimeControl
        virtual bool beginElement(ExceptionCode&);
        virtual bool beginElementAt(float offset, ExceptionCode&);
        virtual bool endElement(ExceptionCode&);
        virtual bool endElementAt(float offset, ExceptionCode&);
        
        static bool attributeIsCSS(const String& attributeName);

protected:
        
        enum CalcMode { CalcModeDiscrete, CalcModeLinear, CalcModePaced, CalcModeSpline };
        CalcMode calcMode() const;
        
        enum AttributeType { AttributeTypeCSS, AttributeTypeXML, AttributeTypeAuto };
        AttributeType attributeType() const;
        
        String toValue() const;
        String byValue() const;
        String fromValue() const;
        
        enum AnimationMode { NoAnimation, ToAnimation, ByAnimation, ValuesAnimation, FromToAnimation, FromByAnimation };
        AnimationMode animationMode() const;

        virtual bool hasValidTarget() const;
        
        String targetAttributeBaseValue() const;
        void setTargetAttributeAnimatedValue(const String&);
        bool targetAttributeIsCSS() const;
        
        bool isAdditive() const;
        bool isAccumulated() const;
    
        // from SVGSMILElement
        virtual void startedActiveInterval();
        virtual void updateAnimation(float percent, unsigned repeat, SVGSMILElement* resultElement);
        virtual void endedActiveInterval();
        
        ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGExternalResourcesRequired, bool, ExternalResourcesRequired, externalResourcesRequired)
private:
        virtual bool calculateFromAndToValues(const String& fromString, const String& toString) = 0;
        virtual bool calculateFromAndByValues(const String& fromString, const String& byString) = 0;
        virtual void calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement) = 0;
        
        void currentValuesForValuesAnimation(float percent, float& effectivePercent, String& from, String& to);

        virtual float calculateDistance(const String& fromString, const String& toString) { return -1.f; }

        void calculateKeyTimesForCalcModePaced();
    
protected:
        bool m_animationValid;

        Vector<String> m_values;
        Vector<float> m_keyTimes;
        Vector<UnitBezier> m_keySplines;
        String m_lastValuesAnimationFrom;
        String m_lastValuesAnimationTo;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAnimationElement_h
