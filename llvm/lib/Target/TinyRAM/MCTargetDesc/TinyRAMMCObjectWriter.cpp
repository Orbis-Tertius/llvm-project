//===-- TinyRAMMCObjectWriter.cpp - TinyRAM ELF writer -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "TinyRAMFixupKinds.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace {

class TinyRAMObjectWriter : public MCELFObjectTargetWriter {
public:
  TinyRAMObjectWriter(uint8_t OSABI);
  ~TinyRAMObjectWriter() override = default;

protected:
  // Override MCELFObjectTargetWriter.
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel) const override;
  bool needsRelocateWithSymbol(const MCSymbol &SD, unsigned Type) const override;
};

} // end anonymous namespace

TinyRAMObjectWriter::TinyRAMObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(
          /*Is64Bit_=*/false,
          OSABI,
          ELF::EM_TINYRAM,
          /*HasRelocationAddend_=*/true) {}

unsigned TinyRAMObjectWriter::getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel)
    const {
  unsigned Type;
  unsigned Kind = static_cast<unsigned>(Fixup.getKind());
  switch (Kind) {
  case TinyRAM::FIXUP_TINYRAM_32:
  case FK_Data_4:
    Type = ELF::R_TINYRAM_32;
    break;
  default:
    llvm_unreachable("Invalid fixup kind!");
  }
  return Type;
}

bool TinyRAMObjectWriter::needsRelocateWithSymbol(const MCSymbol & /*SD*/, unsigned Type) const {
  switch (Type) {
  case ELF::R_TINYRAM_32:
    return true;
  default:
    return false;
  }
}

std::unique_ptr<MCObjectTargetWriter> llvm::createTinyRAMObjectWriter(uint8_t OSABI) {
  return std::make_unique<TinyRAMObjectWriter>(OSABI);
}
