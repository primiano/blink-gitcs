/*
 *  Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *  
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA. 
 *  
 */

#ifndef MathMLInlineContainerElement_h
#define MathMLInlineContainerElement_h

#if ENABLE(MATHML)
#include "MathMLElement.h"

namespace WebCore {
    
class MathMLInlineContainerElement : public MathMLElement {
public:
    static PassRefPtr<MathMLInlineContainerElement> create(const QualifiedName& tagName, Document*);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

protected:
    MathMLInlineContainerElement(const QualifiedName& tagName, Document*);

};
    
}

#endif // ENABLE(MATHML)
#endif // MathMLInlineContainerElement_h
