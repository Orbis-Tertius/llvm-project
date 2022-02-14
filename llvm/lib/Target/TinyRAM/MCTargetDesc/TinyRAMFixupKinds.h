//===-- LanaiFixupKinds.h - Lanai Specific Fixup Entries --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_MCTARGETDESC_LANAIFIXUPKINDS_H
#define LLVM_LIB_TARGET_TINYRAM_MCTARGETDESC_LANAIFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace TinyRAM {

enum Fixups {
  FIXUP_TINYRAM_NONE = FirstTargetFixupKind,

  FIXUP_TINYRAM_32, // general 32-bit relocation

  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // namespace TinyRAM
} // namespace llvm

#endif // LLVM_LIB_TARGET_TINYRAM_MCTARGETDESC_LANAIFIXUPKINDS_H
