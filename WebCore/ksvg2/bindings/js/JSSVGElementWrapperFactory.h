/*
 *  Copyright (C) 2006 Apple Computer, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef JSSVGELEMENTWRAPPERFACTORY_H
#define JSSVGELEMENTWRAPPERFACTORY_H

#ifdef SVG_SUPPORT

#include <wtf/Forward.h>

namespace KJS {
    class DOMNode;
    class ExecState;
}

namespace WebCore {
    class SVGElement;
    KJS::DOMNode* createJSSVGWrapper(KJS::ExecState*, PassRefPtr<SVGElement>);
}

#endif // SVG_SUPPORT

#endif
