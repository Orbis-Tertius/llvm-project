//===-- TinyRAMInstrInfo.h - TinyRAM instruction information -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the TinyRAM implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_TINYRAMINSTRINFO_H
#define LLVM_LIB_TARGET_TINYRAM_TINYRAMINSTRINFO_H

#include "TinyRAM.h"
#include "TinyRAMRegisterInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include <cstdint>

#define GET_INSTRINFO_HEADER
#include "TinyRAMGenInstrInfo.inc"

namespace llvm {

class TinyRAMSubtarget;

class TinyRAMInstrInfo : public TinyRAMGenInstrInfo {
  const TinyRAMRegisterInfo RI;
  TinyRAMSubtarget &STI;

  virtual void anchor();

public:
  explicit TinyRAMInstrInfo(TinyRAMSubtarget &STI);

  // Return the TinyRAMRegisterInfo, which this class owns.
  const TinyRAMRegisterInfo &getRegisterInfo() const {
    return RI;
  }

  void copyPhysReg(
      MachineBasicBlock &MBB,
      MachineBasicBlock::iterator I,
      const DebugLoc &DL,
      MCRegister DestReg,
      MCRegister SrcReg,
      bool KillSrc) const override;

  void storeRegToStackSlot(
      MachineBasicBlock &MBB,
      MachineBasicBlock::iterator MI,
      Register SrcReg,
      bool isKill,
      int FrameIndex,
      const TargetRegisterClass *RC,
      const TargetRegisterInfo *TRI) const override;

  void loadRegFromStackSlot(
      MachineBasicBlock &MBB,
      MachineBasicBlock::iterator MI,
      Register DestReg,
      int FrameIndex,
      const TargetRegisterClass *RC,
      const TargetRegisterInfo *TRI) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_TINYRAM_TINYRAMINSTRINFO_H
