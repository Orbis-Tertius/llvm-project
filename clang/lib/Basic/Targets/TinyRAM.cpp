//===--- TinyRAM.cpp - Implement TinyRAM targets feature
// support-------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements TinyRAM TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "TinyRAM.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/TargetParser.h"
#include <cstdint>
#include <cstring>
#include <limits>

namespace clang {
namespace targets {

TinyRAMTargetInfo::TinyRAMTargetInfo(const llvm::Triple &Triple,
                                     const TargetOptions &)
    : TargetInfo(Triple) {

  std::string Layout = "";

  Layout += "e";

  Layout += "-p:32:32";

  Layout += "-i1:8:32-i8:8:32-i16:16:32-i32:32:32";

  Layout += "-i64:32";

  Layout += "-a:0:32";

  Layout += "-n32";

  resetDataLayout(Layout);

  SizeType = UnsignedInt;
  PtrDiffType = SignedInt;
  IntPtrType = SignedInt;
}

bool TinyRAMTargetInfo::setCPU(const std::string &Name) {
  // no CPU needs to be specified
  return true;
}

void TinyRAMTargetInfo::getTargetDefines(const LangOptions &Opts,
                                         MacroBuilder &Builder) const {
  using llvm::Twine;

  Builder.defineMacro("__tinyRAM__");
}

ArrayRef<Builtin::Info> TinyRAMTargetInfo::getTargetBuiltins() const {
  // FIXME: Implement.
  return None;
}

bool TinyRAMTargetInfo::hasFeature(StringRef Feature) const { return false; }

const char *const TinyRAMTargetInfo::GCCRegNames[] = {
    "r0", "r1", "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};

ArrayRef<const char *> TinyRAMTargetInfo::getGCCRegNames() const {
  return llvm::makeArrayRef(GCCRegNames);
}

ArrayRef<TargetInfo::GCCRegAlias> TinyRAMTargetInfo::getGCCRegAliases() const {
  // No aliases.
  return None;
}

bool TinyRAMTargetInfo::validateAsmConstraint(
    const char *&Name, TargetInfo::ConstraintInfo &info) const {
  return false;
}

const char *TinyRAMTargetInfo::getClobbers() const {
  // FIXME: Is this really right?
  return "";
}

TargetInfo::BuiltinVaListKind TinyRAMTargetInfo::getBuiltinVaListKind() const {
  return TargetInfo::VoidPtrBuiltinVaList;
}

} // namespace targets
} // namespace clang
