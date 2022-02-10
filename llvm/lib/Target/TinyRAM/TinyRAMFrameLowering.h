//===-- TinyRAMFrameLowering.h - Frame lowering for TinyRAM --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_TINYRAMFRAMELOWERING_H
#define LLVM_LIB_TARGET_TINYRAM_TINYRAMFRAMELOWERING_H

#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class TinyRAMTargetMachine;
class TinyRAMSubtarget;

class TinyRAMFrameLowering : public TargetFrameLowering {
public:
  TinyRAMFrameLowering();

  // Override TargetFrameLowering.
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasFP(const MachineFunction &MF) const override;
  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const override;
  MachineBasicBlock::iterator eliminateCallFramePseudoInstr(
      MachineFunction &MF,
      MachineBasicBlock &MBB,
      MachineBasicBlock::iterator I) const override;
};
} // end namespace llvm

#endif
