/*
    Copyright (C) 2004 Richard Moore <rich@kde.org>
    Copyright (C) 2004 Zack Rusin <zack@kde.org>
    Copyright (C) 2005 Frans Englich <frans.englich@telia.com>

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


#ifndef KDOM_XPath_BooleanImpl_H
#define KDOM_XPath_BooleanImpl_H

#include <kdom/xpath/impl/data/ValueImpl.h>

namespace KDOM
{

namespace XPath
{
	class StringImpl;

	class BooleanImpl : public ValueImpl
	{
	public:

		BooleanImpl();
		BooleanImpl(bool value);

    	virtual ~BooleanImpl();

		/**
 		 * Converts the Boolean to a String. See the string() function in TR 4.2 for
 		 * details of the conventions used.
 		 */
		StringImpl  *toString();

		bool value() const;

	
	private:
		bool m_value;
	};
};
};
#endif
// vim:ts=4:noet
