//===-- TinyRAMTargetInfo.cpp - TinyRAM target implementation ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/TinyRAMTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheTinyRAMTarget() {
  static Target TheTinyRAMTarget;
  return TheTinyRAMTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTinyRAMTargetInfo() {
  RegisterTarget<Triple::tinyRAM, /*HasJIT=*/false> X(getTheTinyRAMTarget(), "tinyRAM", "TinyRAM", "TinyRAM");
}
