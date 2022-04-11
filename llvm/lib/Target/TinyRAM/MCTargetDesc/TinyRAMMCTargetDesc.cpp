//===-- TinyRAMMCTargetDesc.cpp - TinyRAM target descriptions ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TinyRAMMCTargetDesc.h"
#include "TargetInfo/TinyRAMTargetInfo.h"
#include "TinyRAMInstPrinter.h"
#include "TinyRAMMCAsmInfo.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "TinyRAMGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "TinyRAMGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "TinyRAMGenRegisterInfo.inc"

static MCAsmInfo *createTinyRAMMCAsmInfo(const MCRegisterInfo &MRI, const Triple &TT, const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new TinyRAMMCAsmInfo(TT);
  return MAI;
}

static MCInstrInfo *createTinyRAMMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitTinyRAMMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createTinyRAMMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitTinyRAMMCRegisterInfo(X, TinyRAM::LR);
  return X;
}

static MCSubtargetInfo *createTinyRAMMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createTinyRAMMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

static MCInstPrinter *createTinyRAMMCInstPrinter(
    const Triple &T,
    unsigned SyntaxVariant,
    const MCAsmInfo &MAI,
    const MCInstrInfo &MII,
    const MCRegisterInfo &MRI) {
  return new TinyRAMInstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTinyRAMTargetMC() {
  // Register the MCAsmInfo.
  TargetRegistry::RegisterMCAsmInfo(getTheTinyRAMTarget(), createTinyRAMMCAsmInfo);

  // Register the MCCodeEmitter.
  TargetRegistry::RegisterMCCodeEmitter(getTheTinyRAMTarget(), createTinyRAMMCCodeEmitter);

  // Register the MCInstrInfo.
  TargetRegistry::RegisterMCInstrInfo(getTheTinyRAMTarget(), createTinyRAMMCInstrInfo);
  // Register the MCRegisterInfo.
  TargetRegistry::RegisterMCRegInfo(getTheTinyRAMTarget(), createTinyRAMMCRegisterInfo);

  // Register the MCSubtargetInfo.
  TargetRegistry::RegisterMCSubtargetInfo(getTheTinyRAMTarget(), createTinyRAMMCSubtargetInfo);
  // Register the MCAsmBackend.
  TargetRegistry::RegisterMCAsmBackend(getTheTinyRAMTarget(), createTinyRAMMCAsmBackend);
  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(getTheTinyRAMTarget(), createTinyRAMMCInstPrinter);
}
