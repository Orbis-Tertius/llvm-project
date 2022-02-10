//===-- TinyRAMMCObjectWriter.cpp - TinyRAM ELF writer -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
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
};

} // end anonymous namespace

TinyRAMObjectWriter::TinyRAMObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(
          /*Is64Bit_=*/false,
          OSABI,
          ELF::EM_88K,
          /*HasRelocationAddend_=*/true) {}

unsigned TinyRAMObjectWriter::getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel)
    const {
  return 0;
}

std::unique_ptr<MCObjectTargetWriter> llvm::createTinyRAMObjectWriter(uint8_t OSABI) {
  return std::make_unique<TinyRAMObjectWriter>(OSABI);
}
