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
#include "TinyRAMISelLowering.h"
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
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
class TinyRAMAsmPrinter : public AsmPrinter {
public:
  explicit TinyRAMAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override {
    return "TinyRAM Assembly Printer";
  }

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &OS) override;
  void emitInstruction(const MachineInstr *MI) override;
  void emitComparison(MCStreamer &Streamer, llvm::Register LHS, llvm::Register RHS, TinyRAMISD::CondCodes CC);
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

void TinyRAMAsmPrinter::emitComparison(
    MCStreamer &Streamer,
    llvm::Register LHS,
    llvm::Register RHS,
    TinyRAMISD::CondCodes CC) {
  using Tcc = TinyRAMISD::CondCodes;

  std::vector<llvm::MCInst> Insts;

  switch (CC) {
  case Tcc::CMPE:
    Insts.push_back(MCInstBuilder(TinyRAM::CMPEr).addReg(LHS).addReg(RHS));
    break;
  case Tcc::CMPA:
    Insts.push_back(MCInstBuilder(TinyRAM::CMPAr).addReg(LHS).addReg(RHS));
    break;
  case Tcc::CMPAE:
    Insts.push_back(MCInstBuilder(TinyRAM::CMPAEr).addReg(LHS).addReg(RHS));
    break;
  case Tcc::CMPG:
    Insts.push_back(MCInstBuilder(TinyRAM::CMPGr).addReg(LHS).addReg(RHS));
    break;
  case Tcc::CMPGE:
    Insts.push_back(MCInstBuilder(TinyRAM::CMPGEr).addReg(LHS).addReg(RHS));
    break;
  case Tcc::CMPNE:
    Insts.push_back(MCInstBuilder(TinyRAM::CMPEr).addReg(LHS).addReg(RHS));
    Insts.push_back(MCInstBuilder(TinyRAM::MOVi).addReg(TinyRAM::R12).addImm(0));
    Insts.push_back(MCInstBuilder(TinyRAM::CMOVi).addReg(TinyRAM::R12).addImm(1));
    Insts.push_back(MCInstBuilder(TinyRAM::CMPEi).addReg(TinyRAM::R12).addImm(0));
    break;
  }

  for (const auto &Inst : Insts) {
    EmitToStreamer(Streamer, Inst);
  }
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
        MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx), MCConstantExpr::create(16, Ctx), Ctx);
    const MCInst MCMI1 = MCInstBuilder(TinyRAM::MOVi).addReg(TinyRAM::LR).addOperand(MCOperand::createExpr(SymbolExpr));

    auto Address = MI->getOperand(0);
    const MCInst MCMI2 = MCInstBuilder(TinyRAM::JMPi).addOperand(Lower.lowerOperand(Address));

    EmitToStreamer(*OutStreamer, MCMI1);
    EmitToStreamer(*OutStreamer, MCMI2);
  } break;

  case TinyRAM::BLA: {
    if (MI->getOperand(0).getType() == MachineOperand::MO_Register) {
      auto &Ctx = MF->getContext();
      TinyRAMMCInstLower Lower(Ctx, *this);
      auto *Sym = Ctx.createNamedTempSymbol();

      OutStreamer->emitLabel(Sym);

      const auto *SymbolExpr = MCBinaryExpr::createAdd(
          MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx), MCConstantExpr::create(16, Ctx), Ctx);
      const MCInst MCMI1 =
          MCInstBuilder(TinyRAM::MOVi).addReg(TinyRAM::LR).addOperand(MCOperand::createExpr(SymbolExpr));

      const MCInst MCMI2 = MCInstBuilder(TinyRAM::JMPr).addReg(MI->getOperand(0).getReg());

      EmitToStreamer(*OutStreamer, MCMI1);
      EmitToStreamer(*OutStreamer, MCMI2);
    } else {
      llvm_unreachable("Unexpected BLA operand");
    }
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
    auto TrueV = MI->getOperand(1).getReg();
    auto FalseV = MI->getOperand(2).getReg();
    auto LHS = MI->getOperand(3).getReg();
    auto RHS = MI->getOperand(4).getReg();
    auto CCInt = (TinyRAMISD::CondCodes)MI->getOperand(5).getImm();

    emitComparison(*OutStreamer, LHS, RHS, CCInt);

    const MCInst MCMI1 = MCInstBuilder(TinyRAM::MOVr).addReg(TinyRAM::R12).addReg(FalseV);
    const MCInst MCMI2 = MCInstBuilder(TinyRAM::CMOVr).addReg(TinyRAM::R12).addReg(TrueV);
    const MCInst MCMI3 = MCInstBuilder(TinyRAM::MOVr).addReg(Dst).addReg(TinyRAM::R12);

    EmitToStreamer(*OutStreamer, MCMI1);
    EmitToStreamer(*OutStreamer, MCMI2);
    EmitToStreamer(*OutStreamer, MCMI3);
  } break;

  case TinyRAM::BRCond: {
    auto &Ctx = MF->getContext();
    TinyRAMMCInstLower Lower(Ctx, *this);

    auto Address = MI->getOperand(0);
    auto LHS = MI->getOperand(1).getReg();
    auto RHS = MI->getOperand(2).getReg();
    auto CCInt = (TinyRAMISD::CondCodes)MI->getOperand(3).getImm();

    emitComparison(*OutStreamer, LHS, RHS, CCInt);

    const MCInst MCMI1 = MCInstBuilder(TinyRAM::CJMPi).addOperand(Lower.lowerOperand(Address));

    EmitToStreamer(*OutStreamer, MCMI1);
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
