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

#ifndef KSVG_SVGTSpanElementImpl_H
#define KSVG_SVGTSpanElementImpl_H

#include "SVGTextPositioningElementImpl.h"

namespace KSVG
{
	class SVGTSpanElementImpl : public SVGTextPositioningElementImpl
	{
	public:
		SVGTSpanElementImpl(KDOM::DocumentImpl *doc, KDOM::NodeImpl::Id id,  const KDOM::DOMString &prefix);
		virtual ~SVGTSpanElementImpl();

		// 'SVGTSpanElement' functions
	};
};

#endif

// vim:ts=4:noet
