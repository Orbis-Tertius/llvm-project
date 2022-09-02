//===-- TinyRAMInstrInfo.td - TinyRAM Instructions ---------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the TinyRAM instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

// Address operands ***********************************************************

def MEMii : Operand<i32> {
  let MIOperandInfo = (ops i32imm, i32imm);
}

def pcrel_imm  : Operand<i32>;

def brtarget : Operand<OtherVT>;

def ADDRspii : ComplexPattern<i32, 2, "SelectADDRspii", [add, frameindex], []>;

// Type profiles **************************************************************

def SDT_Call            : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;

def SDT_TinyRAMWrapper  : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;

def SDT_tinyRAMStwsp    : SDTypeProfile<0, 2, [SDTCisInt<1>]>;

def SDT_TinyRAMBrcond   : SDTypeProfile<0, 4, [SDTCisVT<0, OtherVT>, SDTCisSameAs<1, 2>, SDTCisSameAs<1, 3>]>;

def SDT_TinyRAMSelectCC : SDTypeProfile<1, 5, [SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>, SDTCisSameAs<3, 4>, SDTCisInt<5>]>;


// Nodes **********************************************************************

def SDT_TinyRAMCallSeqStart : SDCallSeqStart<[ SDTCisVT<0, i32>,
                                               SDTCisVT<1, i32> ]>;

def SDT_TinyRAMCallSeqEnd   : SDCallSeqEnd<[ SDTCisVT<0, i32>,
                                             SDTCisVT<1, i32> ]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_TinyRAMCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;

def callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_TinyRAMCallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;


def call             : SDNode<"TinyRAMISD::CALL", SDT_Call,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                              SDNPVariadic]>;

def retflag          : SDNode<"TinyRAMISD::RET_FLAG", SDTNone,
                              [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def tinyRAMStwsp     : SDNode<"TinyRAMISD::STWSP", SDT_tinyRAMStwsp,
                              [SDNPHasChain, SDNPMayStore]>;


def TinyRAMWrapper  : SDNode<"TinyRAMISD::WRAPPER", SDT_TinyRAMWrapper>;

def TinyRAMbrcond   : SDNode<"TinyRAMISD::BRCOND", SDT_TinyRAMBrcond,
                             [SDNPHasChain]>;

def TinyRAMselectcc : SDNode<"TinyRAMISD::SELECT_CC", SDT_TinyRAMSelectCC,
                             []>;


// Pseudo instructions ********************************************************

let Defs = [SP], Uses = [SP] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt, i32imm:$amt2),
                               "# ADJCALLSTACKDOWN $amt, $amt2",
                               [(callseq_start timm:$amt, timm:$amt2)]>;

def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                              "# ADJCALLSTACKUP $amt1",
                              [(callseq_end timm:$amt1, timm:$amt2)]>;
}

def LDWFI  : Pseudo<(outs GPROpnd:$dst), (ins MEMii:$addr),
                    "# LDWFI $dst, $addr",
                    [(set GPROpnd:$dst, (load ADDRspii:$addr))]>;

def LDAWFI : Pseudo<(outs GPROpnd:$dst), (ins MEMii:$addr),
                    "# LDAWFI $dst, $addr",
                    [(set GPROpnd:$dst, ADDRspii:$addr)]>;

def STWFI  : Pseudo<(outs), (ins GPROpnd:$src, MEMii:$addr),
                    "# STWFI $src, $addr",
                    [(store GPROpnd:$src, ADDRspii:$addr)]>;

// applying hasSideEffects is a hack
// without it the instruction is sometimes optimized out
let hasSideEffects=1 in
def SelectCC : Pseudo<
    (outs GPROpnd:$dst),
    (ins GPROpnd:$trueC, GPROpnd:$falseC, GPROpnd:$lhs, GPROpnd:$rhs, i32imm:$cc),
    "# SelectCC PSEUDO",
    [(set i32:$dst, (TinyRAMselectcc i32:$trueC, i32:$falseC, i32:$lhs, i32:$rhs, imm:$cc))]
    >;

let isBranch=1, isIndirectBranch=1, isTerminator=1 in
def BRCond : Pseudo<
    (outs),
    (ins brtarget:$op2, GPROpnd:$lhs, GPROpnd:$rhs, i32imm:$cc),
    "# BRCond PSEUDO",
    [(TinyRAMbrcond bb:$op2, i32:$lhs, i32:$rhs, imm:$cc)]
    >;

let Uses = [SP], mayStore=1 in
def STWSPi : Pseudo<
    (outs),
    (ins GPROpnd:$a, i32imm:$b),
    "# STWSPi",
    [(tinyRAMStwsp GPROpnd:$a, imm:$b)]
    >;

// leftover from TinyRAM
let isReturn=1, isTerminator=1, isBarrier=1, Uses=[LR] in
def RET : Pseudo<
    (outs),
    (ins),
    "# RET",
    [(retflag)]
    >;

let isCall=1,
// All calls clobber the link register and the non-callee-saved registers:
Defs = [R0, R1, R2, R3, LR], Uses = [SP] in {
def BLRF : Pseudo<
    (outs),
    (ins pcrel_imm:$a),
    "# BLRF",
    [(call tglobaladdr:$a)]
    >;

def BLA : Pseudo<
    (outs),
    (ins GPROpnd:$a),
    "# BLA",
    [(call GPROpnd:$a)]
    >;
}

def : Pat<(call texternalsym:$addr), (BLRF texternalsym:$addr)>;

// Real instructions **********************************************************

let isCommutable = 1 in
def ADDr : F_RRR<
   0b00100,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("add", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (add GPROpnd:$op1, GPROpnd:$op2))]
   >;

let isCommutable = 1 in
def ADDi : F_RRI<
   0b00100,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("add", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (add GPROpnd:$op1, imm:$op2))]
   >;

let isCommutable = 1 in
def ANDr : F_RRR<
   0b00000,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("and", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (and GPROpnd:$op1, GPROpnd:$op2))]
   >;

let isCommutable = 1 in
def ORi : F_RRI<
   0b00001,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("or", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (or GPROpnd:$op1, imm:$op2))]
   >;

let isCommutable = 1 in
def ORr : F_RRR<
   0b00001,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("or", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (or GPROpnd:$op1, GPROpnd:$op2))]
   >;

def XORi : F_RRI<
   0b00010,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("xor", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (xor GPROpnd:$op1, imm:$op2))]
   >;

def XORr : F_RRR<
   0b00010,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("xor", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (xor GPROpnd:$op1, GPROpnd:$op2))]
   >;

def SHLi : F_RRI<
   0b01011,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("shl", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (shl GPROpnd:$op1, imm:$op2))]
   >;

def SHLr : F_RRR<
   0b01011,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("shl", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (shl GPROpnd:$op1, GPROpnd:$op2))]
   >;

def SHRi : F_RRI<
   0b01100,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("shr", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (srl GPROpnd:$op1, imm:$op2))]
   >;

def SHRr : F_RRR<
   0b01100,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("shr", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (srl GPROpnd:$op1, GPROpnd:$op2))]
   >;

let isCommutable = 1 in
def ANDi : F_RRI<
   0b00000,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("and", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (and GPROpnd:$op1, imm:$op2))]
   >;

let isCommutable = 1 in
def MULLr : F_RRR<
   0b00110,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("mull", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (mul GPROpnd:$op1, GPROpnd:$op2))]
   >;

let isCommutable = 1 in
def MULLi : F_RRI<
   0b00110,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("mull", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (mul GPROpnd:$op1, imm:$op2))]
   >;

let isCommutable = 1 in
def MULLHUr : F_RRR<
   0b00111,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("umulh", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (mulhu GPROpnd:$op1, GPROpnd:$op2))]
   >;

let isCommutable = 1 in
def MULLHUi : F_RRI<
   0b00111,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("umulh", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (mulhu GPROpnd:$op1, imm:$op2))]
   >;

let isCommutable = 1 in
def MULLHSr : F_RRR<
   0b01000,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("smulh", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (mulhs GPROpnd:$op1, GPROpnd:$op2))]
   >;

let isCommutable = 1 in
def MULLHSi : F_RRI<
   0b01000,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("smulh", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (mulhs GPROpnd:$op1, imm:$op2))]
   >;

def UDIVr : F_RRR<
   0b01001,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("udiv", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (udiv GPROpnd:$op1, GPROpnd:$op2))]
   >;

def UDIVi : F_RRI<
   0b01001,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("udiv", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (udiv GPROpnd:$op1, imm:$op2))]
   >;

def UMODr : F_RRR<
   0b01010,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("umod", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (urem GPROpnd:$op1, GPROpnd:$op2))]
   >;

def UMODi : F_RRI<
   0b01010,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("umod", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (urem GPROpnd:$op1, imm:$op2))]
   >;

def SUBr : F_RRR<
   0b00101,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, GPROpnd:$op2),
   !strconcat("sub", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (sub GPROpnd:$op1, GPROpnd:$op2))]
   >;

def SUBi : F_RRI<
   0b00101,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op1, i32imm:$op2),
   !strconcat("sub", " $dst, $op1, $op2"),
   [(set GPROpnd:$dst, (sub GPROpnd:$op1, imm:$op2))]
   >;

def MOVr : F_RR0<
   0b10010,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op2),
   !strconcat("mov", " $dst, $op2"),
   []
   >;
// TODO: undestand why [(set GPROpnd:$dst, GPROpnd:$op2)] causes cycle

def MOVi : F_RI0<
   0b10010,
   (outs GPROpnd:$dst),
   (ins i32imm:$op2),
   !strconcat("mov", " $dst, $op2"),
   [(set GPROpnd:$dst, imm:$op2)]
   >;

def CMOVr : F_RR0<
   0b10011,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op2),
   !strconcat("cmov", " $dst, $op2"),
   []
   >;

def CMOVi : F_RI0<
   0b10011,
   (outs GPROpnd:$dst),
   (ins i32imm:$op2),
   !strconcat("cmov", " $dst, $op2"),
   []
   >;

let isBranch=1, isIndirectBranch=1, isTerminator=1, isBarrier = 1 in
{
def JMPr : F_0R0<
    0b10100,
    (outs),
    (ins GPROpnd:$op2),
    !strconcat("jmp", " $op2"),
    []
    >;

def JMPi : F_0I0<
    0b10100,
    (outs),
    (ins brtarget:$op2),
    !strconcat("jmp", " $op2"),
    [(br bb:$op2)]
    >;
}

let isBranch=1, isIndirectBranch=1, isTerminator=1 in
{
def CJMPr : F_0R0<
    0b10101,
    (outs),
    (ins GPROpnd:$op2),
    !strconcat("cjmp", " $op2"),
    []
    >;

def CJMPi : F_0I0<
    0b10101,
    (outs),
    (ins brtarget:$op2, GPROpnd:$lhs, GPROpnd:$rhs, i32imm:$cc),
    !strconcat("cjmp", " $op2"),
    []
    >;

def CNJMPr : F_0R0<
    0b10110,
    (outs),
    (ins GPROpnd:$op2),
    !strconcat("cnjmp", " $op2"),
    []
    >;

def CNJMPi : F_0I0<
    0b10110,
    (outs),
    (ins i32imm:$op2),
    !strconcat("cnjmp", " $op2"),
    []
    >;
}

// comparison instructions
let isCompare=1, hasSideEffects=1 in {

let isCommutable = 1 in
def CMPNEr : Pseudo<
    (outs),
    (ins GPROpnd:$op1, GPROpnd:$op2),
     "# CMPNEr PSEUDO",
    []
    >;

let isCommutable = 1 in
def CMPNEi : Pseudo<
    (outs),
    (ins GPROpnd:$op1, i32imm:$op2),
     "# CMPNEi PSEUDO",
    []
    >;

let isCommutable = 1 in
def CMPEr : F_RR0<
    0b01101,
    (outs),
    (ins GPROpnd:$dst, GPROpnd:$op2),
    !strconcat("cmpe", " $dst, $op2"),
    []
    >;

let isCommutable = 1 in
def CMPEi : F_RI0<
    0b01101,
    (outs),
    (ins GPROpnd:$dst, i32imm:$op2),
    !strconcat("cmpe", " $dst, $op2"),
    []
    >;

def CMPAr : F_RR0<
    0b01110,
    (outs),
    (ins GPROpnd:$dst, GPROpnd:$op2),
    !strconcat("cmpa", " $dst, $op2"),
    []
    >;

def CMPAi : F_RI0<
    0b01110,
    (outs),
    (ins GPROpnd:$dst, i32imm:$op2),
    !strconcat("cmpa", " $dst, $op2"),
    []
    >;

def CMPAEr : F_RR0<
    0b01111,
    (outs),
    (ins GPROpnd:$dst, GPROpnd:$op2),
    !strconcat("cmpae", " $dst, $op2"),
    []
    >;

def CMPAEi : F_RI0<
    0b01111,
    (outs),
    (ins GPROpnd:$dst, i32imm:$op2),
    !strconcat("cmpae", " $dst, $op2"),
    []
    >;

def CMPGr : F_RR0<
    0b10000,
    (outs),
    (ins GPROpnd:$dst, GPROpnd:$op2),
    !strconcat("cmpg", " $dst, $op2"),
    []
    >;

def CMPGi : F_RI0<
    0b10000,
    (outs),
    (ins GPROpnd:$dst, i32imm:$op2),
    !strconcat("cmpg", " $dst, $op2"),
    []
    >;

def CMPGEr : F_RR0<
    0b10001,
    (outs),
    (ins GPROpnd:$dst, GPROpnd:$op2),
    !strconcat("cmpge", " $dst, $op2"),
    []
    >;

def CMPGEi : F_RI0<
    0b10001,
    (outs),
    (ins GPROpnd:$dst, i32imm:$op2),
    !strconcat("cmpge", " $dst, $op2"),
    []
    >;

}

// load / store instructions

// i32

let mayLoad=1 in
def LOADr : F_RR0<
   0b11101,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op2),
   !strconcat("load.w", " $dst, $op2"),
   [(set GPROpnd:$dst, (load GPROpnd:$op2))]
   >;

let mayLoad=1 in
def LOADi : F_RI0<
   0b11101,
   (outs GPROpnd:$dst),
   (ins i32imm:$op2),
   !strconcat("load.w", " $dst, $op2"),
   [(set GPROpnd:$dst, (load imm:$op2))]
   >;

let mayStore=1 in
def STOREr : F_RR0<
   0b11100,
   (outs),
   (ins GPROpnd:$op2, GPROpnd:$dst),
   !strconcat("store.w", " $op2, $dst"),
   [(store GPROpnd:$dst, GPROpnd:$op2)]
   >;

let mayStore=1 in
def STOREi : F_RI0<
   0b11100,
   (outs),
   (ins i32imm:$op2, GPROpnd:$dst),
   !strconcat("store.w", " $op2, $dst"),
   [(store GPROpnd:$dst, imm:$op2)]
   >;

// i8

let mayLoad=1 in
def LOADBr : F_RR0<
   0b11011,
   (outs GPROpnd:$dst),
   (ins GPROpnd:$op2),
   !strconcat("load.b", " $dst, $op2"),
   []
   >;

let mayLoad=1 in
def LOADBi : F_RI0<
   0b11011,
   (outs GPROpnd:$dst),
   (ins i32imm:$op2),
   !strconcat("load.b", " $dst, $op2"),
   []
   >;

let mayStore=1 in
def STOREBr : F_RR0<
   0b11010,
   (outs),
   (ins GPROpnd:$op2, GPROpnd:$dst),
   !strconcat("store.b", " $op2, $dst"),
   []
   >;

let mayStore=1 in
def STOREBi : F_RI0<
   0b11010,
   (outs),
   (ins i32imm:$op2, GPROpnd:$dst),
   !strconcat("store.b", " $op2, $dst"),
   []
   >;

// Special instructions

let isBranch=1, isIndirectBranch=1, isTerminator=1, isBarrier = 1 in
{
def ANSWERi : F_0I0<
    0b11111,
    (outs),
    (ins brtarget:$op2),
    !strconcat("answer", " $op2"),
    []
    >;

def ANSWERr : F_0R0<
    0b11111,
    (outs),
    (ins GPROpnd:$op2),
    !strconcat("answer", " $op2"),
    []
    >;
}


// Patterns *******************************************************************

def : Pat<(zextloadi8 imm:$addr), (LOADBi i32imm:$addr)>;
def : Pat<(zextloadi8 GPROpnd:$addr), (LOADBr GPROpnd:$addr)>;

def : Pat<(truncstorei8 GPROpnd:$val, imm:$addr),
          (STOREBi GPROpnd:$addr, i32imm:$val)>;
def : Pat<(truncstorei8 GPROpnd:$val, GPROpnd:$addr),
          (STOREBr GPROpnd:$addr, GPROpnd:$val)>;

def : Pat<(extloadi8 imm:$addr), (LOADBi i32imm:$addr)>;
def : Pat<(extloadi8 GPROpnd:$addr), (LOADBr GPROpnd:$addr)>;

def : Pat<(i32 (TinyRAMWrapper tglobaladdr:$dst)),
          (MOVi tglobaladdr:$dst)>;

