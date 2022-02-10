//===-- TinyRAMRegisterInfo.cpp - TinyRAM Register Information ------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the TinyRAM implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#include "TinyRAMRegisterInfo.h"
#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "TinyRAM.h"
#include "TinyRAMInstrInfo.h"
#include "TinyRAMSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "TinyRAMGenRegisterInfo.inc"

#define DEBUG_TYPE "tinyRAM-reg-info"

TinyRAMRegisterInfo::TinyRAMRegisterInfo() : TinyRAMGenRegisterInfo(TinyRAM::LR) {}

const MCPhysReg *TinyRAMRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg CalleeSavedRegsFP[] = {// LR
                                                TinyRAM::LR,

                                                // other callee-saved
                                                TinyRAM::R4,
                                                TinyRAM::R5,
                                                TinyRAM::R6,
                                                TinyRAM::R7,
                                                TinyRAM::R8,
                                                TinyRAM::R9,
                                                TinyRAM::R10,
                                                TinyRAM::R11,
                                                0};

  return CalleeSavedRegsFP;
}

BitVector TinyRAMRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  Reserved.set(TinyRAM::SP);  // SP
  Reserved.set(TinyRAM::LR);  // LR
  Reserved.set(TinyRAM::FP);  // FP
  Reserved.set(TinyRAM::R12); // scratch register used in prologue & epilogue

  return Reserved;
}

void TinyRAMRegisterInfo::eliminateFrameIndex(
    MachineBasicBlock::iterator II,
    int SPAdj,
    unsigned FIOperandNum,
    RegScavenger *RS) const {
  assert(RS == nullptr && "Unexpected register scavenger");
  assert(SPAdj == 0 && "Unexpected SPAdj");

  MachineInstr &MI = *II;
  MachineOperand &FrameOp = MI.getOperand(FIOperandNum);
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc Dl = MI.getDebugLoc();

  const int StackSize = MF.getFrameInfo().getStackSize();
  const int Offset1 = MF.getFrameInfo().getObjectOffset(FrameOp.getIndex());

#ifndef NDEBUG
  LLVM_DEBUG(errs() << "\nFunction         : " << MF.getName() << "\n");
  LLVM_DEBUG(errs() << "<--------->\n");
  LLVM_DEBUG(MI.print(errs()));
  LLVM_DEBUG(errs() << "FrameIndex         : " << FrameOp.getIndex() << "\n");
  LLVM_DEBUG(errs() << "FrameOffset        : " << Offset1 << "\n");
  LLVM_DEBUG(errs() << "StackSize          : " << StackSize << "\n");
#endif

  // fold constant into offset.
  const int Offset2 = Offset1 + MI.getOperand(FIOperandNum + 1).getImm();

  assert(Offset2 % 4 == 0 && "Misaligned stack offset");
  LLVM_DEBUG(errs() << "Offset : " << Offset2 << "\n");

  const Register Reg = MI.getOperand(0).getReg();
  const TinyRAMInstrInfo &TII = *static_cast<const TinyRAMInstrInfo *>(MF.getSubtarget().getInstrInfo());

  auto FrameReg = getFrameRegister(MF);

  int Offset3 = Offset2;
  int Op = TinyRAM::ADDi;
  if (Offset2 < 0) {
    Offset3 = -Offset2;
    Op = TinyRAM::SUBi;
  }

  switch (MI.getOpcode()) {
  case TinyRAM::LDWFI:
    BuildMI(MBB, II, Dl, TII.get(Op)).addReg(TinyRAM::R12).addReg(FrameReg).addImm(Offset3);

    BuildMI(MBB, II, Dl, TII.get(TinyRAM::LOADr)).addReg(Reg).addReg(TinyRAM::R12);
    break;

  case TinyRAM::LDAWFI:
    // TODO: should not offset be multiplied
    BuildMI(MBB, II, Dl, TII.get(Op)).addReg(Reg).addReg(FrameReg).addImm(Offset3);

    break;

  case TinyRAM::STWFI:
    BuildMI(MBB, II, Dl, TII.get(Op)).addReg(TinyRAM::R12).addReg(FrameReg).addImm(Offset3);

    BuildMI(MBB, II, Dl, TII.get(TinyRAM::STOREr)).addReg(TinyRAM::R12).addReg(Reg);
    break;

  default:
    llvm_unreachable("Unexpected Opcode");
  }

  // Erase old instruction.
  MBB.erase(II);
}

Register TinyRAMRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return TinyRAM::FP;
}
