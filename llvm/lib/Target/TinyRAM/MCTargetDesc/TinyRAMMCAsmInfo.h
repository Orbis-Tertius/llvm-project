//====-- TinyRAMMCAsmInfo.h - TinyRAM asm properties ---------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_MCTARGETDESC_TINYRAMMCASMINFO_H
#define LLVM_LIB_TARGET_TINYRAM_MCTARGETDESC_TINYRAMMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
class Triple;

class TinyRAMMCAsmInfo : public MCAsmInfoELF {
public:
  explicit TinyRAMMCAsmInfo(const Triple &TT);
};

} // end namespace llvm

#endif
