/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSInterfaceJIT_h
#define JSInterfaceJIT_h

#include "JITCode.h"
#include "JITStubs.h"
#include "JSImmediate.h"
#include "MacroAssembler.h"
#include "RegisterFile.h"
#include <wtf/AlwaysInline.h>
#include <wtf/Vector.h>

namespace JSC {
    class JSInterfaceJIT : public MacroAssembler {
    public:
        // NOTES:
        //
        // regT0 has two special meanings.  The return value from a stub
        // call will always be in regT0, and by default (unless
        // a register is specified) emitPutVirtualRegister() will store
        // the value from regT0.
        //
        // regT3 is required to be callee-preserved.
        //
        // tempRegister2 is has no such dependencies.  It is important that
        // on x86/x86-64 it is ecx for performance reasons, since the
        // MacroAssembler will need to plant register swaps if it is not -
        // however the code will still function correctly.
#if CPU(X86_64)
        static const RegisterID returnValueRegister = X86Registers::eax;
        static const RegisterID cachedResultRegister = X86Registers::eax;
        static const RegisterID firstArgumentRegister = X86Registers::edi;
        
        static const RegisterID timeoutCheckRegister = X86Registers::r12;
        static const RegisterID callFrameRegister = X86Registers::r13;
        static const RegisterID tagTypeNumberRegister = X86Registers::r14;
        static const RegisterID tagMaskRegister = X86Registers::r15;
        
        static const RegisterID regT0 = X86Registers::eax;
        static const RegisterID regT1 = X86Registers::edx;
        static const RegisterID regT2 = X86Registers::ecx;
        static const RegisterID regT3 = X86Registers::ebx;
        
        static const FPRegisterID fpRegT0 = X86Registers::xmm0;
        static const FPRegisterID fpRegT1 = X86Registers::xmm1;
        static const FPRegisterID fpRegT2 = X86Registers::xmm2;
#elif CPU(X86)
        static const RegisterID returnValueRegister = X86Registers::eax;
        static const RegisterID cachedResultRegister = X86Registers::eax;
        // On x86 we always use fastcall conventions = but on
        // OS X if might make more sense to just use regparm.
        static const RegisterID firstArgumentRegister = X86Registers::ecx;
        
        static const RegisterID timeoutCheckRegister = X86Registers::esi;
        static const RegisterID callFrameRegister = X86Registers::edi;
        
        static const RegisterID regT0 = X86Registers::eax;
        static const RegisterID regT1 = X86Registers::edx;
        static const RegisterID regT2 = X86Registers::ecx;
        static const RegisterID regT3 = X86Registers::ebx;
        
        static const FPRegisterID fpRegT0 = X86Registers::xmm0;
        static const FPRegisterID fpRegT1 = X86Registers::xmm1;
        static const FPRegisterID fpRegT2 = X86Registers::xmm2;
#elif CPU(ARM_THUMB2)
        static const RegisterID returnValueRegister = ARMRegisters::r0;
        static const RegisterID cachedResultRegister = ARMRegisters::r0;
        static const RegisterID firstArgumentRegister = ARMRegisters::r0;
        
        static const RegisterID regT0 = ARMRegisters::r0;
        static const RegisterID regT1 = ARMRegisters::r1;
        static const RegisterID regT2 = ARMRegisters::r2;
        static const RegisterID regT3 = ARMRegisters::r4;
        
        static const RegisterID callFrameRegister = ARMRegisters::r5;
        static const RegisterID timeoutCheckRegister = ARMRegisters::r6;
        
        static const FPRegisterID fpRegT0 = ARMRegisters::d0;
        static const FPRegisterID fpRegT1 = ARMRegisters::d1;
        static const FPRegisterID fpRegT2 = ARMRegisters::d2;
#elif CPU(ARM_TRADITIONAL)
        static const RegisterID returnValueRegister = ARMRegisters::r0;
        static const RegisterID cachedResultRegister = ARMRegisters::r0;
        static const RegisterID firstArgumentRegister = ARMRegisters::r0;
        
        static const RegisterID timeoutCheckRegister = ARMRegisters::r5;
        static const RegisterID callFrameRegister = ARMRegisters::r4;
        
        static const RegisterID regT0 = ARMRegisters::r0;
        static const RegisterID regT1 = ARMRegisters::r1;
        static const RegisterID regT2 = ARMRegisters::r2;
        // Callee preserved
        static const RegisterID regT3 = ARMRegisters::r7;
        
        static const RegisterID regS0 = ARMRegisters::S0;
        // Callee preserved
        static const RegisterID regS1 = ARMRegisters::S1;
        
        static const RegisterID regStackPtr = ARMRegisters::sp;
        static const RegisterID regLink = ARMRegisters::lr;
        
        static const FPRegisterID fpRegT0 = ARMRegisters::d0;
        static const FPRegisterID fpRegT1 = ARMRegisters::d1;
        static const FPRegisterID fpRegT2 = ARMRegisters::d2;
#elif CPU(MIPS)
        static const RegisterID returnValueRegister = MIPSRegisters::v0;
        static const RegisterID cachedResultRegister = MIPSRegisters::v0;
        static const RegisterID firstArgumentRegister = MIPSRegisters::a0;
        
        // regT0 must be v0 for returning a 32-bit value.
        static const RegisterID regT0 = MIPSRegisters::v0;
        
        // regT1 must be v1 for returning a pair of 32-bit value.
        static const RegisterID regT1 = MIPSRegisters::v1;
        
        static const RegisterID regT2 = MIPSRegisters::t4;
        
        // regT3 must be saved in the callee, so use an S register.
        static const RegisterID regT3 = MIPSRegisters::s2;
        
        static const RegisterID callFrameRegister = MIPSRegisters::s0;
        static const RegisterID timeoutCheckRegister = MIPSRegisters::s1;
        
        static const FPRegisterID fpRegT0 = MIPSRegisters::f4;
        static const FPRegisterID fpRegT1 = MIPSRegisters::f6;
        static const FPRegisterID fpRegT2 = MIPSRegisters::f8;
#else
#error "JIT not supported on this platform."
#endif

        inline Jump emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID payload);
        inline Jump emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst);

#if USE(JSVALUE32_64)
        inline Jump emitJumpIfNotJSCell(unsigned virtualRegisterIndex);
        inline Address tagFor(unsigned index, RegisterID base = callFrameRegister);
#endif
        inline Address payloadFor(unsigned index, RegisterID base = callFrameRegister);
        inline Address addressFor(unsigned index, RegisterID base = callFrameRegister);
    };
    
#if USE(JSVALUE32_64)
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID payload)
    {
        loadPtr(payloadFor(virtualRegisterIndex), payload);
        return emitJumpIfNotJSCell(virtualRegisterIndex);
    }

    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitJumpIfNotJSCell(unsigned virtualRegisterIndex)
    {
        return branch32(NotEqual, tagFor(virtualRegisterIndex), Imm32(JSValue::CellTag));
    }
    
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst)
    {
        loadPtr(payloadFor(virtualRegisterIndex), dst);
        return branch32(NotEqual, tagFor(virtualRegisterIndex), Imm32(JSValue::Int32Tag));
    }
    
    inline JSInterfaceJIT::Address JSInterfaceJIT::tagFor(unsigned index, RegisterID base)
    {
        return Address(base, (index * sizeof(Register)) + OBJECT_OFFSETOF(JSValue, u.asBits.tag));
    }
    
    inline JSInterfaceJIT::Address JSInterfaceJIT::payloadFor(unsigned index, RegisterID base)
    {
        return Address(base, (index * sizeof(Register)) + OBJECT_OFFSETOF(JSValue, u.asBits.payload));
    }
#endif

#if USE(JSVALUE64)
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID dst)
    {
        loadPtr(addressFor(virtualRegisterIndex), dst);
        return branchTestPtr(NonZero, dst, tagMaskRegister);
    }
    
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst)
    {
        loadPtr(addressFor(virtualRegisterIndex), dst);
        Jump result = branchPtr(Below, dst, tagTypeNumberRegister);
        zeroExtend32ToPtr(dst, dst);
        return result;
    }
#endif

#if USE(JSVALUE32)
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID dst)
    {
        loadPtr(addressFor(virtualRegisterIndex), dst);
        return branchTest32(NonZero, dst, Imm32(JSImmediate::TagMask));
    }

    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst)
    {
        loadPtr(addressFor(virtualRegisterIndex), dst);
        Jump result = branchTest32(Zero, dst, Imm32(JSImmediate::TagTypeNumber));
        rshift32(Imm32(JSImmediate::IntegerPayloadShift), dst);
        return result;
    }    
#endif

#if !USE(JSVALUE32_64)
    inline JSInterfaceJIT::Address JSInterfaceJIT::payloadFor(unsigned index, RegisterID base)
    {
        return addressFor(index, base);
    }
#endif

    inline JSInterfaceJIT::Address JSInterfaceJIT::addressFor(unsigned index, RegisterID base)
    {
        return Address(base, (index * sizeof(Register)));
    }

}

#endif // JSInterfaceJIT_h
