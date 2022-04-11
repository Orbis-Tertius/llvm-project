//===-- TinyRAMSubtarget.cpp - TinyRAM Subtarget Information --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TinyRAMSubtarget.h"
#include "TinyRAM.h"
#include "TinyRAMISelLowering.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "tinyRAM-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "TinyRAMGenSubtargetInfo.inc"

void TinyRAMSubtarget::anchor() {}

TinyRAMSubtarget::TinyRAMSubtarget(
    const Triple &TT,
    const std::string &CPU,
    const std::string &FS,
    const TargetMachine &TM)
    : TinyRAMGenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS), TargetTriple(TT), InstrInfo(*this),
      TLInfo(createTargetLowering(TM, *this)), FrameLowering() {}
