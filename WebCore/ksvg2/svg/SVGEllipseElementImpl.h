/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KSVG_SVGEllipseElementImpl_H
#define KSVG_SVGEllipseElementImpl_H
#if SVG_SUPPORT

#include "SVGTestsImpl.h"
#include "SVGLangSpaceImpl.h"
#include "SVGStyledTransformableElementImpl.h"
#include "SVGExternalResourcesRequiredImpl.h"

namespace WebCore
{
    class SVGAnimatedLengthImpl;
    class SVGEllipseElementImpl : public SVGStyledTransformableElementImpl,
                                  public SVGTestsImpl,
                                  public SVGLangSpaceImpl,
                                  public SVGExternalResourcesRequiredImpl
    {
    public:
        SVGEllipseElementImpl(const QualifiedName& tagName, DocumentImpl *doc);
        virtual ~SVGEllipseElementImpl();
        
        virtual bool isValid() const { return SVGTestsImpl::isValid(); }

        // 'SVGEllipseElement' functions
        SVGAnimatedLengthImpl *cx() const;
        SVGAnimatedLengthImpl *cy() const;
        SVGAnimatedLengthImpl *rx() const;
        SVGAnimatedLengthImpl *ry() const;

        virtual void parseMappedAttribute(MappedAttributeImpl *attr);

        virtual bool rendererIsNeeded(RenderStyle *) { return true; }
        virtual KCanvasPath* toPathData() const;

        virtual const SVGStyledElementImpl *pushAttributeContext(const SVGStyledElementImpl *context);

    private:
        mutable RefPtr<SVGAnimatedLengthImpl> m_cx;
        mutable RefPtr<SVGAnimatedLengthImpl> m_cy;
        mutable RefPtr<SVGAnimatedLengthImpl> m_rx;
        mutable RefPtr<SVGAnimatedLengthImpl> m_ry;
    };
};

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
