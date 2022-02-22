//===-- TinyRAMFrameLowering.cpp - Frame lowering for TinyRAM ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TinyRAMFrameLowering.h"
#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "TinyRAMRegisterInfo.h"
#include "TinyRAMSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

TinyRAMFrameLowering::TinyRAMFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0, Align(4), false /* StackRealignable */) {}

void TinyRAMFrameLowering::emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const {

  assert(hasFP(MF));
  assert(&MF.front() == &MBB && "Shrink-wrapping not supported");

  MachineBasicBlock::iterator MBBI = MBB.begin();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const TinyRAMInstrInfo &TII = *MF.getSubtarget<TinyRAMSubtarget>().getInstrInfo();

  DebugLoc Dl;

  if (MFI.getMaxAlign() > getStackAlign()) {
    report_fatal_error("emitPrologue unsupported alignment: " + Twine(MFI.getMaxAlign().value()));
  }

  const AttributeList &PAL = MF.getFunction().getAttributes();
  if (PAL.hasAttrSomewhere(Attribute::Nest)) {
    llvm_unreachable("Nest attribute unreachable");
  }

  assert(MFI.getStackSize() % 4 == 0 && "Misaligned frame size");
  const int FrameSize = MFI.getStackSize();

  // Save frame pointer
  MBB.addLiveIn(TinyRAM::FP);
  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::SUBi))
      .addReg(TinyRAM::R12)
      .addReg(TinyRAM::SP)
      .addImm(4)
      .setMIFlag(MachineInstr::FrameSetup);

  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::STOREr))
      .addReg(TinyRAM::R12)
      .addReg(TinyRAM::FP)
      .setMIFlag(MachineInstr::FrameSetup);

  // Set the FP from the SP.
  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::MOVr))
      .addReg(TinyRAM::FP)
      .addReg(TinyRAM::SP)
      .setMIFlag(MachineInstr::FrameSetup);

  // Allocate space on the stack
  MBB.addLiveIn(TinyRAM::SP);
  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::SUBi))
      .addReg(TinyRAM::SP)
      .addReg(TinyRAM::SP)
      .addImm(FrameSize)
      .setMIFlag(MachineInstr::FrameSetup);

  // TODO: check register liveness
}

void TinyRAMFrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  const TinyRAMInstrInfo &TII = *MF.getSubtarget<TinyRAMSubtarget>().getInstrInfo();

  DebugLoc Dl;

  // Set the SP from the FP.
  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::MOVr))
      .addReg(TinyRAM::SP)
      .addReg(TinyRAM::FP)
      .setMIFlag(MachineInstr::FrameDestroy);

  // Restore frame pointer
  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::SUBi))
      .addReg(TinyRAM::R12)
      .addReg(TinyRAM::SP)
      .addImm(4)
      .setMIFlag(MachineInstr::FrameDestroy);

  BuildMI(MBB, MBBI, Dl, TII.get(TinyRAM::LOADr))
      .addReg(TinyRAM::FP)
      .addReg(TinyRAM::R12)
      .setMIFlag(MachineInstr::FrameDestroy);
}

bool TinyRAMFrameLowering::hasFP(const MachineFunction &MF) const {
  return true;
}

void TinyRAMFrameLowering::determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  MachineFrameInfo &MFI = MF.getFrameInfo();

  SavedRegs.set(TinyRAM::LR);

  int Offset = -4;

  // reserve for FP
  MFI.CreateFixedObject(4, Offset, true);
}

// This function eliminates ADJCALLSTACKDOWN,
// ADJCALLSTACKUP pseudo instructions
MachineBasicBlock::iterator TinyRAMFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF,
    MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {

  const TinyRAMInstrInfo &TII = *MF.getSubtarget<TinyRAMSubtarget>().getInstrInfo();

  if (!hasReservedCallFrame(MF)) {
    // Turn the adjcallstackdown instruction into 'extsp <amt>' and the
    // adjcallstackup instruction into 'ldaw sp, sp[<amt>]'
    MachineInstr &Old = *I;
    uint64_t Amount = Old.getOperand(0).getImm();
    if (Amount != 0) {
      // We need to keep the stack aligned properly.  To do this, we round the
      // amount of space needed for the outgoing arguments up to the next
      // alignment boundary. Amount is in bytes
      Amount = alignTo(Amount, getStackAlign());

      assert(Amount % 4 == 0);

      MachineInstr *New;
      if (Old.getOpcode() == TinyRAM::ADJCALLSTACKDOWN) {
        New = BuildMI(MF, Old.getDebugLoc(), TII.get(TinyRAM::SUBi))
                  .addReg(TinyRAM::SP)
                  .addReg(TinyRAM::SP)
                  .addImm(Amount);
      } else {
        assert(Old.getOpcode() == TinyRAM::ADJCALLSTACKUP);
        New = BuildMI(MF, Old.getDebugLoc(), TII.get(TinyRAM::ADDi))
                  .addReg(TinyRAM::SP)
                  .addReg(TinyRAM::SP)
                  .addImm(Amount);
      }

      // Replace the pseudo instruction with a new instruction...
      MBB.insert(I, New);
    }
  }

  return MBB.erase(I);
}