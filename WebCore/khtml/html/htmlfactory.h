/*
 * This file is part of the HTML DOM implementation for KDE.
 *
 * Copyright (C) 2005, 2006 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef HTMLELEMENTFACTORY_H
#define HTMLELEMENTFACTORY_H

namespace KXMLCore {
    template <typename T> class PassRefPtr;
}
using KXMLCore::PassRefPtr;

namespace WebCore {

class AtomicString;
class DocumentImpl;
class ElementImpl;
class HTMLElementImpl;
class HTMLFormElementImpl;
class QualifiedName;

// The idea behind this class is that there will eventually be a mapping from namespace URIs to ElementFactories that can dispense elements.
// In a compound document world, the generic createElement function (will end up being virtual) will be called.
class HTMLElementFactory {
public:
    PassRefPtr<ElementImpl> createElement(const QualifiedName&, DocumentImpl*, bool createdByParser = true);
    static PassRefPtr<HTMLElementImpl> createHTMLElement(const AtomicString& tagName, DocumentImpl*, HTMLFormElementImpl* = 0, bool createdByParser = true);
};

}

#endif
