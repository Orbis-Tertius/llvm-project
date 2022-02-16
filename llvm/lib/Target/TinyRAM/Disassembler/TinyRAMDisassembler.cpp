//===-- TinyRAMDisassembler.cpp - Disassembler for TinyRAM --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "TargetInfo/TinyRAMTargetInfo.h"
#include "TinyRAM.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

#define DEBUG_TYPE "TinyRAM-disassembler"

using DecodeStatus = MCDisassembler::DecodeStatus;

namespace {

class TinyRAMDisassembler : public MCDisassembler {
public:
  TinyRAMDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx) : MCDisassembler(STI, Ctx) {}
  ~TinyRAMDisassembler() override = default;

  DecodeStatus getInstruction(
      MCInst &instr,
      uint64_t &Size,
      ArrayRef<uint8_t> Bytes,
      uint64_t Address,
      raw_ostream &CStream) const override;
};

} // end anonymous namespace

static MCDisassembler *createTinyRAMDisassembler(const Target &T, const MCSubtargetInfo &STI, MCContext &Ctx) {
  return new TinyRAMDisassembler(STI, Ctx);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTinyRAMDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(getTheTinyRAMTarget(), createTinyRAMDisassembler);
}

template <unsigned N>
static DecodeStatus decodeUImmOperand(MCInst &Inst, uint64_t Imm) {
  if (!isUInt<N>(Imm))
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static const uint16_t GPRDecoderTable[] = {
    TinyRAM::R0,
    TinyRAM::R1,
    TinyRAM::R2,
    TinyRAM::R3,
    TinyRAM::R4,
    TinyRAM::R5,
    TinyRAM::R6,
    TinyRAM::R7,
    TinyRAM::R8,
    TinyRAM::R9,
    TinyRAM::R10,
    TinyRAM::R11,
    TinyRAM::R12,
    TinyRAM::FP,
    TinyRAM::LR,
    TinyRAM::SP};

static DecodeStatus DecodeGPRRegisterClass(MCInst &Inst, uint64_t RegNo, uint64_t Address, const void *Decoder) {
  if (RegNo > 15)
    return MCDisassembler::Fail;
  unsigned Register = GPRDecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

#include "TinyRAMGenDisassemblerTables.inc"

DecodeStatus TinyRAMDisassembler::getInstruction(
    MCInst &MI,
    uint64_t &Size,
    ArrayRef<uint8_t> Bytes,
    uint64_t Address,
    raw_ostream &CS) const {
  // Instruction size is always 64 bit.
  if (Bytes.size() < 8) {
    Size = 0;
    return MCDisassembler::Fail;
  }
  Size = 8;

  // Construct the instruction.
  uint64_t Inst = 0;
  for (uint32_t I = 0; I < Size; ++I) {
    Inst |= ((uint64_t)Bytes[I]) << (I * 8);
  }

  return decodeInstruction(DecoderTableTinyRAM64, MI, Inst, Address, this, STI);
}
