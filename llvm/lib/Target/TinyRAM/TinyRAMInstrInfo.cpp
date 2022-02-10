//===-- TinyRAMInstrInfo.cpp - TinyRAM instruction information -----------===//
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

#include "TinyRAMInstrInfo.h"
#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "TinyRAM.h"
#include "TinyRAMRegisterInfo.h"
#include "TinyRAMSubtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/BranchProbability.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cstdint>
#include <iterator>

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#define GET_INSTRMAP_INFO
#include "TinyRAMGenInstrInfo.inc"

#define DEBUG_TYPE "tinyRAM-ii"

// Pin the vtable to this file.
void TinyRAMInstrInfo::anchor() {}

TinyRAMInstrInfo::TinyRAMInstrInfo(TinyRAMSubtarget &STI)
    : TinyRAMGenInstrInfo(TinyRAM::ADJCALLSTACKDOWN, TinyRAM::ADJCALLSTACKUP), RI(), STI(STI) {}

void TinyRAMInstrInfo::copyPhysReg(
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I,
    const DebugLoc &DL,
    MCRegister DestReg,
    MCRegister SrcReg,
    bool KillSrc) const {
  bool GRDest = TinyRAM::GPRRegClass.contains(DestReg);
  bool GRSrc = TinyRAM::GPRRegClass.contains(SrcReg);

  if (GRDest && GRSrc) {
    BuildMI(MBB, I, DL, get(TinyRAM::MOVr), DestReg).addReg(SrcReg, getKillRegState(KillSrc));
  } else {
    llvm_unreachable("Impossible reg-to-reg copy");
  }
}

void TinyRAMInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I,
    Register SrcReg,
    bool IsKill,
    int FrameIndex,
    const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  // As of https://groups.google.com/g/llvm-dev/c/JtuPiwjG6UY
  // Attaching MachineMemOperand seems to be used for optimization
  // Thus I dare to not attach it at all

  DebugLoc DL;
  if (I != MBB.end() && !I->isDebugInstr())
    DL = I->getDebugLoc();

  BuildMI(MBB, I, DL, get(TinyRAM::STWFI)).addReg(SrcReg, getKillRegState(IsKill)).addFrameIndex(FrameIndex).addImm(0);
}

void TinyRAMInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I,
    Register DestReg,
    int FrameIndex,
    const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end() && !I->isDebugInstr())
    DL = I->getDebugLoc();

  BuildMI(MBB, I, DL, get(TinyRAM::LDWFI), DestReg).addFrameIndex(FrameIndex).addImm(0);
}
