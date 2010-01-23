/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"

#include "FontSelector.h"
#include "MathMLNames.h"
#include "RenderInline.h"
#include "RenderText.h"

namespace WebCore {
    
using namespace MathMLNames;
    
RenderMathMLBlock::RenderMathMLBlock(Node* container) 
    : RenderBlock(container) 
{
}

bool RenderMathMLBlock::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->node() && child->node()->nodeType() == Node::ELEMENT_NODE;
}

PassRefPtr<RenderStyle> RenderMathMLBlock::makeBlockStyle()
{
    RefPtr<RenderStyle> newStyle = RenderStyle::create();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(BLOCK);
    return newStyle;
}

int RenderMathMLBlock::nonOperatorHeight() const
{
    if (!isRenderMathMLOperator())
        return offsetHeight();
        
    return 0;
}

void RenderMathMLBlock::stretchToHeight(int height) 
{
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
       if (current->isRenderMathMLBlock()) {
          RenderMathMLBlock* block = toRenderMathMLBlock(current);
          block->stretchToHeight(height);
       }
}


}    

#endif
