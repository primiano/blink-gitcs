/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003, 2006 Apple Computer, Inc.
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

#include "config.h"
#include "DOMParser.h"

#include "DOMImplementation.h"
#include "PlatformString.h"

namespace WebCore {
    
PassRefPtr<Document> DOMParser::parseFromString(const String& str, const String& contentType)
{
    if (!DOMImplementation::isXMLMIMEType(contentType))
        return 0;

    RefPtr<Document> doc = DOMImplementation::instance()->createDocument(contentType, 0, false);

    doc->open();
    doc->write(str);
    doc->finishParsing();
    doc->close();
        
    return doc.release();
}

}
