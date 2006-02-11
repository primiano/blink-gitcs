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

#ifndef KSVG_SVGLangSpaceImpl_H
#define KSVG_SVGLangSpaceImpl_H
#if SVG_SUPPORT

#include "AtomicString.h"

namespace WebCore
{
    class DOMStringImpl;
    class MappedAttributeImpl;
};

namespace WebCore
{
    class SVGAnimatedLengthImpl;
    class SVGAnimatedStringImpl;

    class SVGLangSpaceImpl
    {
    public:
        SVGLangSpaceImpl();
        virtual ~SVGLangSpaceImpl();

        // 'SVGLangSpace' functions
        const AtomicString& xmllang() const;
        void setXmllang(const AtomicString& xmlLang);

        const AtomicString& xmlspace() const;
        void setXmlspace(const AtomicString& xmlSpace);

        bool parseMappedAttribute(MappedAttributeImpl *attr);

    private:
        AtomicString m_lang;
        AtomicString m_space;
    };
};

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
