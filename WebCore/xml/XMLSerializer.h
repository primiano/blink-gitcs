/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003, 2006 Apple Computer, Inc.
 *  Copyright (C) 2006 Samuel Weinig (sam@webkit.org)
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

#ifndef XMLSerializer_h
#define XMLSerializer_h

#include "Shared.h"
#include "PlatformString.h"

namespace WebCore {

    typedef int ExceptionCode;

    class Node;

    class XMLSerializer : public Shared<XMLSerializer> {
    public:
        String serializeToString(Node*, ExceptionCode&);
    };

} // namespace WebCore

#endif // XMLSerializer_h
