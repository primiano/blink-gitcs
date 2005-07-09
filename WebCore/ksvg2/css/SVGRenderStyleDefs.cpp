/*
    Copyright (C) 2004 Nikolas Zimmermann <wildfox@kde.org>
				  2004 Rob Buis <buis@kde.org>

    Based on khtml code by:
    Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
    Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
    Copyright (C) 2002-2003 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Apple Computer, Inc.

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

#include "ksvg.h"
#include "SVGRenderStyle.h"
#include "SVGRenderStyleDefs.h"

using namespace KSVG;

StyleFillData::StyleFillData() : KDOM::Shared(false)
{
	paint = SVGRenderStyle::initialFillPaint();
	opacity = SVGRenderStyle::initialFillOpacity();
}

StyleFillData::StyleFillData(const StyleFillData &other) : KDOM::Shared(false)
{
	paint = other.paint;
	opacity = other.opacity;
}

bool StyleFillData::operator==(const StyleFillData &other) const
{
	if(opacity != other.opacity)
		return false;

	if(!paint || !other.paint)
		return paint == other.paint;

	if(paint->paintType() != other.paint->paintType())
		return false;

	if(paint->paintType() == SVG_PAINTTYPE_URI)
		return paint->uri() == other.paint->uri();

	if(paint->paintType() == SVG_PAINTTYPE_RGBCOLOR)
		return paint->color() == other.paint->color();

	return (paint == other.paint) && (opacity == other.opacity);
}

StyleStrokeData::StyleStrokeData() : KDOM::Shared(false)
{
	width = SVGRenderStyle::initialStrokeWidth();
	paint = SVGRenderStyle::initialStrokePaint();
	opacity = SVGRenderStyle::initialStrokeOpacity();
	miterLimit = SVGRenderStyle::initialStrokeMiterLimit();
	dashOffset = SVGRenderStyle::initialStrokeDashOffset();
	dashArray = SVGRenderStyle::initialStrokeDashArray();
}

StyleStrokeData::StyleStrokeData(const StyleStrokeData &other) : KDOM::Shared(false)
{
	width = other.width;
	paint = other.paint;
	opacity = other.opacity;
	miterLimit = other.miterLimit;
	dashOffset = other.dashOffset;
	dashArray = other.dashArray;
}

bool StyleStrokeData::operator==(const StyleStrokeData &other) const
{
    return (paint == other.paint) &&
		   (width == other.width) &&
		   (opacity == other.opacity) &&
		   (miterLimit == other.miterLimit) &&
		   (dashOffset == other.dashOffset) &&
		   (dashArray == other.dashArray);
}

StyleStopData::StyleStopData() : KDOM::Shared(false)
{
	color = SVGRenderStyle::initialStopColor();
	opacity = SVGRenderStyle::initialStopOpacity();
}

StyleStopData::StyleStopData(const StyleStopData &other) : KDOM::Shared(false)
{
	color = other.color;
	opacity = other.opacity;
}

bool StyleStopData::operator==(const StyleStopData &other) const
{
    return (color == other.color) &&
		   (opacity == other.opacity);
}

StyleClipData::StyleClipData() : KDOM::Shared(false)
{
	clipPath = SVGRenderStyle::initialClipPath();
}

StyleClipData::StyleClipData(const StyleClipData &other) : KDOM::Shared(false)
{
	clipPath = other.clipPath;
}

bool StyleClipData::operator==(const StyleClipData &other) const
{
    return (clipPath == other.clipPath);
}

StyleMarkerData::StyleMarkerData() : KDOM::Shared(false)
{
	startMarker = SVGRenderStyle::initialStartMarker();
	midMarker = SVGRenderStyle::initialMidMarker();
	endMarker = SVGRenderStyle::initialEndMarker();
}

StyleMarkerData::StyleMarkerData(const StyleMarkerData &other) : KDOM::Shared(false)
{
	startMarker = other.startMarker;
	midMarker = other.midMarker;
	endMarker = other.endMarker;
}

bool StyleMarkerData::operator==(const StyleMarkerData &other) const
{
    return (startMarker == other.startMarker && midMarker == other.midMarker && endMarker == other.endMarker);
}

StyleMiscData::StyleMiscData() : KDOM::Shared(false)
{
	opacity = SVGRenderStyle::initialOpacity();
	floodColor = SVGRenderStyle::initialColor();
	floodOpacity = SVGRenderStyle::initialOpacity();
}

StyleMiscData::StyleMiscData(const StyleMiscData &other) : KDOM::Shared(false)
{
	opacity = other.opacity;
	filter = other.filter;
	floodColor = other.floodColor;
	floodOpacity = other.floodOpacity;
}

bool StyleMiscData::operator==(const StyleMiscData &other) const
{
	return (opacity == other.opacity && filter == other.filter &&
			floodOpacity == other.floodOpacity && floodColor == other.floodColor);
}

// vim:ts=4:noet
