//===-- TinyRAMTargetMachine.h - Define TargetMachine for TinyRAM ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the TinyRAM specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_TINYRAMTARGETMACHINE_H
#define LLVM_LIB_TARGET_TINYRAM_TINYRAMTARGETMACHINE_H

#include "TinyRAMSubtarget.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class TinyRAMTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<TinyRAMSubtarget>> SubtargetMap;

public:
  TinyRAMTargetMachine(
      const Target &T,
      const Triple &TT,
      StringRef CPU,
      StringRef FS,
      const TargetOptions &Options,
      Optional<Reloc::Model> RM,
      Optional<CodeModel::Model> CM,
      CodeGenOpt::Level OL,
      bool JIT);
  ~TinyRAMTargetMachine() override;
  const TinyRAMSubtarget *getSubtargetImpl(const Function &) const override;

  // DO NOT IMPLEMENT: There is no such thing as a valid default subtarget,
  // subtargets are per-function entities based on the target-specific
  // attributes of each function.
  const TinyRAMSubtarget *getSubtargetImpl() const = delete;

  // Override LLVMTargetMachine
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  // TargetTransformInfo getTargetTransformInfo(const Function &F) override;
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // end namespace llvm

#endif
