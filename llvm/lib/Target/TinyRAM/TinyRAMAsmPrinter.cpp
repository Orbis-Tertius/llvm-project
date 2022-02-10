//===-- TinyRAMAsmPrinter.cpp - TinyRAM LLVM assembly writer -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format TinyRAM assembly language.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/TinyRAMInstPrinter.h"
#include "MCTargetDesc/TinyRAMMCTargetDesc.h"
#include "TargetInfo/TinyRAMTargetInfo.h"
#include "TinyRAM.h"
#include "TinyRAMInstrInfo.h"
#include "TinyRAMMCInstLower.h"
#include "TinyRAMTargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfoImpls.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
class TinyRAMAsmPrinter : public AsmPrinter {
#if 0
    TinyRAMTargetStreamer &getTargetStreamer() {
      return static_cast<TinyRAMTargetStreamer &>(
          *OutStreamer->getTargetStreamer());
    }
#endif
public:
  explicit TinyRAMAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override {
    return "TinyRAM Assembly Printer";
  }

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &OS) override;
  void emitInstruction(const MachineInstr *MI) override;
};
} // end of anonymous namespace

bool TinyRAMAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &OS) {
  if (ExtraCode)
    return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS);
  TinyRAMMCInstLower Lower(MF->getContext(), *this);
  MCOperand MO(Lower.lowerOperand(MI->getOperand(OpNo)));
  TinyRAMInstPrinter::printOperand(MO, MAI, OS);
  return false;
}

void TinyRAMAsmPrinter::emitInstruction(const MachineInstr *MI) {
  switch (MI->getOpcode()) {
  case TinyRAM::RET: {
    MCInst LoweredMI;
    LoweredMI = MCInstBuilder(TinyRAM::JMPr).addReg(TinyRAM::LR);
    EmitToStreamer(*OutStreamer, LoweredMI);
  } break;

  case TinyRAM::BLRF: {
    auto &Ctx = MF->getContext();
    TinyRAMMCInstLower Lower(Ctx, *this);
    auto *Sym = Ctx.createNamedTempSymbol();

    OutStreamer->emitLabel(Sym);

    const auto *SymbolExpr = MCBinaryExpr::createAdd(
        MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx), MCConstantExpr::create(8, Ctx), Ctx);
    const MCInst MCMI1 = MCInstBuilder(TinyRAM::MOVi).addReg(TinyRAM::LR).addOperand(MCOperand::createExpr(SymbolExpr));

    auto Address = MI->getOperand(0);
    const MCInst MCMI2 = MCInstBuilder(TinyRAM::JMPi).addOperand(Lower.lowerOperand(Address));

    EmitToStreamer(*OutStreamer, MCMI1);
    EmitToStreamer(*OutStreamer, MCMI2);
  } break;

  case TinyRAM::STWSPi: {
    auto Reg = MI->getOperand(0).getReg();
    auto Offset = MI->getOperand(1).getImm();

    const MCInst MCMI1 = MCInstBuilder(TinyRAM::ADDi).addReg(TinyRAM::R12).addReg(TinyRAM::SP).addImm(Offset);
    const MCInst MCMI2 = MCInstBuilder(TinyRAM::STOREr).addReg(TinyRAM::R12).addReg(Reg);

    EmitToStreamer(*OutStreamer, MCMI1);
    EmitToStreamer(*OutStreamer, MCMI2);

  } break;

  case TinyRAM::SelectCC: {
    auto Dst = MI->getOperand(0).getReg();
    auto Reg1 = MI->getOperand(1).getReg();
    auto Reg2 = MI->getOperand(2).getReg();

    const MCInst MCMI1 = MCInstBuilder(TinyRAM::MOVr).addReg(Dst).addReg(Reg2);
    const MCInst MCMI2 = MCInstBuilder(TinyRAM::CMOVr).addReg(Dst).addReg(Reg1);

    EmitToStreamer(*OutStreamer, MCMI1);
    EmitToStreamer(*OutStreamer, MCMI2);
  } break;

  case TinyRAM::CMPNEi:
  case TinyRAM::CMPNEr: {
    auto Op1 = MI->getOperand(0).getReg();
    auto Op2 = MI->getOperand(1);

    if (Op2.isReg()) {
      auto MI = MCInstBuilder(TinyRAM::CMPEr).addReg(Op1).addReg(Op2.getReg());
      EmitToStreamer(*OutStreamer, MI);
    } else if (Op2.isImm()) {
      auto MI = MCInstBuilder(TinyRAM::CMPEr).addReg(Op1).addImm(Op2.getImm());
      EmitToStreamer(*OutStreamer, MI);
    } else {
      llvm_unreachable("CMPNE: Unsupported operand type");
    }

    // now invert the flag

    auto MI1 = MCInstBuilder(TinyRAM::MOVi).addReg(TinyRAM::R12).addImm(0);
    auto MI2 = MCInstBuilder(TinyRAM::CMOVi).addReg(TinyRAM::R12).addImm(1);
    auto MI3 = MCInstBuilder(TinyRAM::CMPEi).addReg(TinyRAM::R12).addImm(0);

    EmitToStreamer(*OutStreamer, MI1);
    EmitToStreamer(*OutStreamer, MI2);
    EmitToStreamer(*OutStreamer, MI3);
  } break;

  default: {
    MCInst LoweredMI;
    TinyRAMMCInstLower Lower(MF->getContext(), *this);
    Lower.lower(MI, LoweredMI);
    // doLowerInstr(MI, LoweredMI);
    EmitToStreamer(*OutStreamer, LoweredMI);
  } break;
  }
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTinyRAMAsmPrinter() {
  RegisterAsmPrinter<TinyRAMAsmPrinter> X(getTheTinyRAMTarget());
}
