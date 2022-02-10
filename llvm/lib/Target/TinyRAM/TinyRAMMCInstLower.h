//===-- TinyRAMMCInstLower.h - Lower MachineInstr to MCInst ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_TINYRAMMCINSTLOWER_H
#define LLVM_LIB_TARGET_TINYRAM_TINYRAMMCINSTLOWER_H

#include "llvm/MC/MCExpr.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {
class AsmPrinter;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineOperand;
class Mangler;

class LLVM_LIBRARY_VISIBILITY TinyRAMMCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  TinyRAMMCInstLower(MCContext &Ctx, AsmPrinter &Printer);

  // Lower MachineInstr MI to MCInst OutMI.
  void lower(const MachineInstr *MI, MCInst &OutMI) const;

  // Return an MCOperand for MO.
  MCOperand lowerOperand(const MachineOperand &MO) const;

  // Return an MCExpr for symbolic operand MO with variant kind Kind.
  const MCExpr *getExpr(const MachineOperand &MO, MCSymbolRefExpr::VariantKind Kind) const;
};
} // end namespace llvm

#endif
