/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WREC.h"

#if ENABLE(WREC)

#include "CharacterClassConstructor.h"
#include "Interpreter.h"
#include "WRECFunctors.h"
#include "WRECParser.h"
#include "pcre_internal.h"

#define __ assembler. 

using namespace WTF;

namespace JSC { namespace WREC {

// This limit comes from the limit set in PCRE
static const int MaxPatternSize = (1 << 16);

CompiledRegExp compileRegExp(Interpreter* interpreter, const UString& pattern, unsigned* numSubpatterns_ptr, const char** error_ptr, bool ignoreCase, bool multiline)
{
    if (pattern.size() > MaxPatternSize) {
        *error_ptr = "Regular expression too large.";
        return 0;
    }

    X86Assembler assembler(interpreter->assemblerBuffer());
    Parser parser(pattern, ignoreCase, multiline, assembler);
    Generator& generator = parser.generator();
    JmpSrcVector failures;

    generator.generateEnter();
    generator.generateSaveIndex();

    Generator::JmpDst beginPattern = __ label();
    if (!parser.parsePattern(failures)) {
        *error_ptr = "Regular expression malformed.";
        return 0;
    }
    generator.generateReturnSuccess();

    __ link(failures, __ label());
    generator.generateIncrementIndex();
    generator.generateLoopIfNotEndOfInput(beginPattern);
    generator.generateReturnFailure();

    *numSubpatterns_ptr = parser.numSubpatterns();
    return reinterpret_cast<CompiledRegExp>(__ executableCopy());
}

} } // namespace JSC::WREC

#endif // ENABLE(WREC)
