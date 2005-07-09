/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2005 Frans Englich 	<frans.englich@telia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef KDOM_XPathNSResolverImpl_H
#define KDOM_XPathNSResolverImpl_H

#include <kdom/Shared.h>

namespace KDOM
{
	class DOMString;

	class XPathNSResolverImpl: public Shared
	{
	public:

		virtual DOMString lookupNamespaceURI(const DOMString& prefix) const;

		XPathNSResolverImpl();
		XPathNSResolverImpl(NodeImpl *nodeResolver);

	private:

		NodeImpl *m_node;
	};
};

#endif
// vim:ts=4:noet
