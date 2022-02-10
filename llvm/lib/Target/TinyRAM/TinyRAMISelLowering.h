//===-- TinyRAMISelLowering.h - TinyRAM DAG lowering interface -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that TinyRAM uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_TINYRAMISELLOWERING_H
#define LLVM_LIB_TARGET_TINYRAM_TINYRAMISELLOWERING_H

#include "TinyRAM.h"
#include "TinyRAMInstrInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

#include <memory>

namespace llvm {

class TinyRAMSubtarget;
class TinyRAMSubtarget;

namespace TinyRAMISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  // Return with a flag operand.  Operand 0 is the chain operand.
  RET_FLAG,

  // Calls a function.  Operand 0 is the chain operand and operand 1
  // is the target address.  The arguments start at operand 2.
  // There is an optional glue operand at the end.
  CALL,

  // Stores a word on the stack, using a constant offset from the stack pointer.
  // The offset is specified in bytes and must be divisible by word size (4).
  // STWSP is used to write to stack variables.
  STWSP,

  WRAPPER,

  CMPE,
  CMPA,
  CMPAE,
  CMPG,
  CMPGE,
  CMPNE,

  BRCOND,

  SELECT_CC,
};
} // end namespace TinyRAMISD

std::unique_ptr<TargetLowering> createTargetLowering(const TargetMachine &TM, const TinyRAMSubtarget &STI);

} // end namespace llvm

#endif
