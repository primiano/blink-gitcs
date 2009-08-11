/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EvalCodeCache_h
#define EvalCodeCache_h

#include "JSGlobalObject.h"
#include "Nodes.h"
#include "Parser.h"
#include "SourceCode.h"
#include "UString.h"
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace JSC {

    class EvalCodeCache {
    public:
        PassRefPtr<EvalNode> get(ExecState* exec, const UString& evalSource, ScopeChainNode* scopeChain, JSValue& exceptionValue)
        {
            RefPtr<EvalNode> evalNode;

            if (evalSource.size() < maxCacheableSourceLength && (*scopeChain->begin())->isVariableObject())
                evalNode = m_cacheMap.get(evalSource.rep());

            if (!evalNode) {
                int errorLine;
                UString errorMessage;
                
                SourceCode source = makeSource(evalSource);
                evalNode = exec->globalData().parser->parse<EvalNode>(exec, exec->dynamicGlobalObject()->debugger(), source, &errorLine, &errorMessage);
                if (evalNode) {
                    if (evalSource.size() < maxCacheableSourceLength && (*scopeChain->begin())->isVariableObject() && m_cacheMap.size() < maxCacheEntries)
                        m_cacheMap.set(evalSource.rep(), evalNode);
                } else {
                    exceptionValue = Error::create(exec, SyntaxError, errorMessage, errorLine, source.provider()->asID(), 0);
                    return 0;
                }
            }

            return evalNode.release();
        }

        bool isEmpty() const { return m_cacheMap.isEmpty(); }

        void markAggregate(MarkStack& markStack)
        {
            EvalCacheMap::iterator end = m_cacheMap.end();
            for (EvalCacheMap::iterator ptr = m_cacheMap.begin(); ptr != end; ++ptr)
                ptr->second->markAggregate(markStack);
        }
    private:
        static const int maxCacheableSourceLength = 256;
        static const int maxCacheEntries = 64;

        typedef HashMap<RefPtr<UString::Rep>, RefPtr<EvalNode> > EvalCacheMap;
        EvalCacheMap m_cacheMap;
    };

} // namespace JSC

#endif // EvalCodeCache_h
