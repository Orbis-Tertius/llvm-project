//===-- TinyRAMTargetMachine.cpp - Define TargetMachine for TinyRAM -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "TinyRAMTargetMachine.h"
#include "TargetInfo/TinyRAMTargetInfo.h"
#include "TinyRAM.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTinyRAMTarget() {
  // Register the target.
  RegisterTargetMachine<TinyRAMTargetMachine> X(getTheTinyRAMTarget());
}

namespace {
// TODO: Check.
std::string computeDataLayout(const Triple &TT, StringRef CPU, StringRef FS) {
  std::string Ret;

  Ret += "e";

  Ret += "-p:32:32";

  Ret += "-i1:8:32-i8:8:32-i16:16:32-i32:32:32";

  Ret += "-i64:32";

  Ret += "-a:32:32";

  Ret += "-n32";

  return Ret;
}

// TODO: Check.
Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue() || *RM == Reloc::DynamicNoPIC)
    return Reloc::Static;
  return *RM;
}

} // namespace

TinyRAMTargetMachine::TinyRAMTargetMachine(
    const Target &T,
    const Triple &TT,
    StringRef CPU,
    StringRef FS,
    const TargetOptions &Options,
    Optional<Reloc::Model> RM,
    Optional<CodeModel::Model> CM,
    CodeGenOpt::Level OL,
    bool JIT)
    : LLVMTargetMachine(
          T,
          computeDataLayout(TT, CPU, FS),
          TT,
          CPU,
          FS,
          Options,
          getEffectiveRelocModel(RM),
          getEffectiveCodeModel(CM, CodeModel::Medium),
          OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()) {
  initAsmInfo();
}

TinyRAMTargetMachine::~TinyRAMTargetMachine() {}

const TinyRAMSubtarget *TinyRAMTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None) ? CPUAttr.getValueAsString().str() : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None) ? FSAttr.getValueAsString().str() : TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<TinyRAMSubtarget>(TargetTriple, CPU, FS, *this);
  }

  return I.get();
}

namespace {
/// TinyRAM Code Generator Pass Configuration Options.
class TinyRAMPassConfig : public TargetPassConfig {
public:
  TinyRAMPassConfig(TinyRAMTargetMachine &TM, PassManagerBase &PM) : TargetPassConfig(TM, PM) {}

  TinyRAMTargetMachine &getTinyRAMTargetMachine() const {
    return getTM<TinyRAMTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *TinyRAMTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new TinyRAMPassConfig(*this, PM);
}

bool TinyRAMPassConfig::addInstSelector() {
  addPass(createTinyRAMISelDag(getTinyRAMTargetMachine(), getOptLevel()));
  return false;
}

void TinyRAMPassConfig::addPreEmitPass() {
  // TODO Add pass for div-by-zero check.
}
