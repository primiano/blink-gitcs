/*
    Copyright (C) 2004 Nikolas Zimmermann <wildfox@kde.org>
				  2004 Rob Buis <buis@kde.org>

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

#include "DOMString.h"
#include "CounterImpl.h"

using namespace KDOM;

CounterImpl::CounterImpl() : Shared(true)
{
	m_listStyle = 0;
}

CounterImpl::~CounterImpl()
{
}

DOMString CounterImpl::identifier() const
{
	return m_identifier;
}

void CounterImpl::setIdentifier(const DOMString &value)
{
	m_identifier = value;
}

unsigned int CounterImpl::listStyle() const
{
	return m_listStyle;
}

void CounterImpl::setListStyle(unsigned int value)
{
	m_listStyle = value;
}

DOMString CounterImpl::separator() const
{
	return m_separator;
}

void CounterImpl::setSeparator(const DOMString &value)
{
	m_separator = value;
}

// vim:ts=4:noet
