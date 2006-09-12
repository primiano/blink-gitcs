/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 */

#include "config.h"
#ifdef SVG_SUPPORT
#include "DeprecatedStringList.h"

#include "Attr.h"

#include <kcanvas/KCanvasFilters.h>
#include <kcanvas/device/KRenderingDevice.h>

#include "SVGNames.h"
#include "SVGHelper.h"
#include "SVGRenderStyle.h"
#include "SVGColor.h"
#include "SVGFELightElement.h"
#include "SVGFESpecularLightingElement.h"


using namespace WebCore;

SVGFESpecularLightingElement::SVGFESpecularLightingElement(const QualifiedName& tagName, Document *doc)
    : SVGFilterPrimitiveStandardAttributes(tagName, doc)
    , m_specularConstant(0.0)
    , m_specularExponent(0.0)
    , m_surfaceScale(0.0)
    , m_lightingColor(new SVGColor())
    , m_kernelUnitLengthX(0.0)
    , m_kernelUnitLengthY(0.0)
    , m_filterEffect(0)
{
}

SVGFESpecularLightingElement::~SVGFESpecularLightingElement()
{
    delete m_filterEffect;
}

ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, String, String, string, In1, in1, SVGNames::inAttr.localName(), m_in1)
ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, double, Number, number, SpecularConstant, specularConstant, SVGNames::specularConstantAttr.localName(), m_specularConstant)
ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, double, Number, number, SpecularExponent, specularExponent, SVGNames::specularExponentAttr.localName(), m_specularExponent)
ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, double, Number, number, SurfaceScale, surfaceScale, SVGNames::surfaceScaleAttr.localName(), m_surfaceScale)
ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, double, Number, number, KernelUnitLengthX, kernelUnitLengthX, "kernelUnitLengthX", m_kernelUnitLengthX)
ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, double, Number, number, KernelUnitLengthY, kernelUnitLengthY, "kernelUnitLengthY", m_kernelUnitLengthY)
ANIMATED_PROPERTY_DEFINITIONS(SVGFESpecularLightingElement, SVGColor*, Color, color, LightingColor, lightingColor, SVGNames::lighting_colorAttr.localName(), m_lightingColor.get())

void SVGFESpecularLightingElement::parseMappedAttribute(MappedAttribute *attr)
{    
    const String& value = attr->value();
    if (attr->name() == SVGNames::inAttr)
        setIn1BaseValue(value);
    else if (attr->name() == SVGNames::surfaceScaleAttr)
        setSurfaceScaleBaseValue(value.deprecatedString().toDouble());
    else if (attr->name() == SVGNames::specularConstantAttr)
        setSpecularConstantBaseValue(value.deprecatedString().toDouble());
    else if (attr->name() == SVGNames::specularExponentAttr)
        setSpecularExponentBaseValue(value.deprecatedString().toDouble());
    else if (attr->name() == SVGNames::kernelUnitLengthAttr) {
        DeprecatedStringList numbers = DeprecatedStringList::split(' ', value.deprecatedString());
        setKernelUnitLengthXBaseValue(numbers[0].toDouble());
        if (numbers.count() == 1)
            setKernelUnitLengthYBaseValue(numbers[0].toDouble());
        else
            setKernelUnitLengthYBaseValue(numbers[1].toDouble());
    } else if (attr->name() == SVGNames::lighting_colorAttr)
        setLightingColorBaseValue(new SVGColor(value));
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

KCanvasFESpecularLighting *SVGFESpecularLightingElement::filterEffect() const
{
    if (!m_filterEffect) 
        m_filterEffect = static_cast<KCanvasFESpecularLighting *>(renderingDevice()->createFilterEffect(FE_SPECULAR_LIGHTING));
    m_filterEffect->setIn(in1());
    setStandardAttributes(m_filterEffect);
    m_filterEffect->setSpecularConstant((specularConstant()));
    m_filterEffect->setSpecularExponent((specularExponent()));
    m_filterEffect->setSurfaceScale((surfaceScale()));
    m_filterEffect->setKernelUnitLengthX((kernelUnitLengthX()));
    m_filterEffect->setKernelUnitLengthY((kernelUnitLengthY()));
    m_filterEffect->setLightingColor(lightingColor()->color());
    updateLights();
    return m_filterEffect;
}

void SVGFESpecularLightingElement::updateLights() const
{
    if (!m_filterEffect)
        return;

    KCLightSource *light = 0;    
    for (Node *n = firstChild(); n; n = n->nextSibling()) {
        if (n->hasTagName(SVGNames::feDistantLightTag)||n->hasTagName(SVGNames::fePointLightTag)||n->hasTagName(SVGNames::feSpotLightTag)) {
            SVGFELightElement *lightNode = static_cast<SVGFELightElement *>(n); 
            light = lightNode->lightSource();
            break;
        }
    }
    m_filterEffect->setLightSource(light);
}
#endif // SVG_SUPPORT

