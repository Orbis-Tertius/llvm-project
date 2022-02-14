//===-- TinyRAMMCAsmBackend.cpp - TinyRAM assembler backend --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//#include "MCTargetDesc/TinyRAMMCFixups.h"
#include "MCTargetDesc/TinyRAMFixupKinds.h"
#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class TinyRAMMCAsmBackend : public MCAsmBackend {
  uint8_t OSABI;

public:
  TinyRAMMCAsmBackend(uint8_t osABI) : MCAsmBackend(support::big), OSABI(osABI) {}

  // Override MCAsmBackend
  unsigned getNumFixupKinds() const override {
    return TinyRAM::NumTargetFixupKinds;
  }
  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;
  void applyFixup(
      const MCAssembler &Asm,
      const MCFixup &Fixup,
      const MCValue &Target,
      MutableArrayRef<char> Data,
      uint64_t Value,
      bool IsResolved,
      const MCSubtargetInfo *STI) const override;
  bool mayNeedRelaxation(const MCInst &Inst, const MCSubtargetInfo &STI) const override {
    return false;
  }
  bool fixupNeedsRelaxation(
      const MCFixup &Fixup,
      uint64_t Value,
      const MCRelaxableFragment *Fragment,
      const MCAsmLayout &Layout) const override {
    return false;
  }
  bool writeNopData(raw_ostream &OS, uint64_t Count) const override;
  std::unique_ptr<MCObjectTargetWriter> createObjectTargetWriter() const override {
    return createTinyRAMObjectWriter(OSABI);
  }
};
} // end anonymous namespace

const MCFixupKindInfo &TinyRAMMCAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  static const MCFixupKindInfo Infos[TinyRAM::NumTargetFixupKinds] = {
      {"FIXUP_LANAI_32", 32, 32, 0},
  };

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() && "Invalid kind!");
  return Infos[Kind - FirstTargetFixupKind];
}

void TinyRAMMCAsmBackend::applyFixup(
    const MCAssembler &Asm,
    const MCFixup &Fixup,
    const MCValue &Target,
    MutableArrayRef<char> Data,
    uint64_t Value,
    bool IsResolved,
    const MCSubtargetInfo *STI) const {
  if (!Value)
    return;

  llvm_unreachable("Fixups should be always turned into relocations");
}

bool TinyRAMMCAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count) const {
  for (uint64_t I = 0; I != Count; ++I)
    OS << '\x7';
  return true;
}

MCAsmBackend *llvm::createTinyRAMMCAsmBackend(
    const Target &T,
    const MCSubtargetInfo &STI,
    const MCRegisterInfo &MRI,
    const MCTargetOptions &Options) {
  uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(STI.getTargetTriple().getOS());
  return new TinyRAMMCAsmBackend(OSABI);
}
