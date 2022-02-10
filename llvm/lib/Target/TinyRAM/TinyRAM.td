//===-- TinyRAM.td - Describe the M88000 Target Machine -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// TinyRAM Subtarget features.
//

//===----------------------------------------------------------------------===//
// Register File, Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "TinyRAMRegisterInfo.td"
include "TinyRAMCallingConv.td"
include "TinyRAMSchedule.td"
include "TinyRAMInstrFormats.td"
include "TinyRAMInstrInfo.td"

//===----------------------------------------------------------------------===//
// TinyRAM processors supported.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"v1", TinyRAMSchedModel, []>;

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def TinyRAMInstrInfo : InstrInfo;
def TinyRAMAsmParser : AsmParser;
def TinyRAMAsmParserVariant : AsmParserVariant {
  let RegisterPrefix = "%";
}

def TinyRAMAsmWriter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  int PassSubtarget = 1;
  int Variant = 0;
}

def TinyRAM : Target {
  let InstructionSet = TinyRAMInstrInfo;
  let AssemblyParsers  = [TinyRAMAsmParser];
  let AssemblyParserVariants = [TinyRAMAsmParserVariant];
  let AssemblyWriters = [TinyRAMAsmWriter];
  let AllowRegisterRenaming = 1;
}
