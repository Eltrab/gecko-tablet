/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_x86_CodeGenerator_x86_h
#define jit_x86_CodeGenerator_x86_h

#include "jit/x86-shared/CodeGenerator-x86-shared.h"
#include "jit/x86/Assembler-x86.h"

namespace js {
namespace jit {

class OutOfLineTruncate;
class OutOfLineTruncateFloat32;

class CodeGeneratorX86 : public CodeGeneratorX86Shared
{
  private:
    CodeGeneratorX86* thisFromCtor() {
        return this;
    }

  protected:
    ValueOperand ToValue(LInstruction* ins, size_t pos);
    ValueOperand ToOutValue(LInstruction* ins);
    ValueOperand ToTempValue(LInstruction* ins, size_t pos);

    void load(Scalar::Type vt, const Operand& srcAddr, const LDefinition* out);
    void loadI64(Scalar::Type vt, const Operand& srcAddr, const Register64 out);
    void store(Scalar::Type vt, const LAllocation* value, const Operand& dstAddr);
    void storeI64(Scalar::Type vt, const LInt64Allocation value, const Operand& dstAddr);

    void loadSimd(Scalar::Type type, unsigned numElems, const Operand& srcAddr, FloatRegister out);
    void emitSimdLoad(LAsmJSLoadHeap* ins);

    void storeSimd(Scalar::Type type, unsigned numElems, FloatRegister in, const Operand& dstAddr);
    void emitSimdStore(LAsmJSStoreHeap* ins);

    void memoryBarrier(MemoryBarrierBits barrier);

    template <typename T>
    void emitWasmLoad(T* ins);
    template <typename T>
    void emitWasmStore(T* ins);

  public:
    CodeGeneratorX86(MIRGenerator* gen, LIRGraph* graph, MacroAssembler* masm);

  public:
    void visitBox(LBox* box);
    void visitBoxFloatingPoint(LBoxFloatingPoint* box);
    void visitUnbox(LUnbox* unbox);
    void visitValue(LValue* value);
    void visitCompareB(LCompareB* lir);
    void visitCompareBAndBranch(LCompareBAndBranch* lir);
    void visitCompareBitwise(LCompareBitwise* lir);
    void visitCompareBitwiseAndBranch(LCompareBitwiseAndBranch* lir);
    void visitAsmJSUInt32ToDouble(LAsmJSUInt32ToDouble* lir);
    void visitAsmJSUInt32ToFloat32(LAsmJSUInt32ToFloat32* lir);
    void visitTruncateDToInt32(LTruncateDToInt32* ins);
    void visitTruncateFToInt32(LTruncateFToInt32* ins);
    void visitLoadTypedArrayElementStatic(LLoadTypedArrayElementStatic* ins);
    void visitStoreTypedArrayElementStatic(LStoreTypedArrayElementStatic* ins);
    void emitAsmJSCall(LAsmJSCallBase* ins);
    void visitAsmJSCall(LAsmJSCall* ins);
    void visitAsmJSCallI64(LAsmJSCallI64* ins);
    void visitWasmLoad(LWasmLoad* ins);
    void visitWasmLoadI64(LWasmLoadI64* ins);
    void visitWasmStore(LWasmStore* ins);
    void visitWasmStoreI64(LWasmStoreI64* ins);
    void visitWasmLoadGlobalVar(LWasmLoadGlobalVar* ins);
    void visitWasmLoadGlobalVarI64(LWasmLoadGlobalVarI64* ins);
    void visitWasmStoreGlobalVar(LWasmStoreGlobalVar* ins);
    void visitWasmStoreGlobalVarI64(LWasmStoreGlobalVarI64* ins);
    void visitAsmJSLoadHeap(LAsmJSLoadHeap* ins);
    void visitAsmJSStoreHeap(LAsmJSStoreHeap* ins);
    void visitAsmJSCompareExchangeHeap(LAsmJSCompareExchangeHeap* ins);
    void visitAsmJSAtomicExchangeHeap(LAsmJSAtomicExchangeHeap* ins);
    void visitAsmJSAtomicBinopHeap(LAsmJSAtomicBinopHeap* ins);
    void visitAsmJSAtomicBinopHeapForEffect(LAsmJSAtomicBinopHeapForEffect* ins);
    void visitWasmTruncateToInt32(LWasmTruncateToInt32* ins);

    void visitOutOfLineTruncate(OutOfLineTruncate* ool);
    void visitOutOfLineTruncateFloat32(OutOfLineTruncateFloat32* ool);

    void visitCompareI64(LCompareI64* lir);
    void visitCompareI64AndBranch(LCompareI64AndBranch* lir);
    void visitDivOrModI64(LDivOrModI64* lir);
    void visitUDivOrModI64(LUDivOrModI64* lir);
    void visitAsmSelectI64(LAsmSelectI64* lir);
    void visitAsmReinterpretFromI64(LAsmReinterpretFromI64* lir);
    void visitAsmReinterpretToI64(LAsmReinterpretToI64* lir);
    void visitExtendInt32ToInt64(LExtendInt32ToInt64* lir);
    void visitWrapInt64ToInt32(LWrapInt64ToInt32* lir);
    void visitClzI64(LClzI64* lir);
    void visitCtzI64(LCtzI64* lir);
    void visitNotI64(LNotI64* lir);
    void visitWasmTruncateToInt64(LWasmTruncateToInt64* lir);
    void visitInt64ToFloatingPoint(LInt64ToFloatingPoint* lir);
    void visitTestI64AndBranch(LTestI64AndBranch* lir);

  private:
    void asmJSAtomicComputeAddress(Register addrTemp, Register ptrReg,
                                   const MWasmMemoryAccess* access);
};

typedef CodeGeneratorX86 CodeGeneratorSpecific;

} // namespace jit
} // namespace js

#endif /* jit_x86_CodeGenerator_x86_h */
