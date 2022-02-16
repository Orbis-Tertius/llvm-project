//===-- TinyRAMInstrFormats.td - TinyRAM Instruction Formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class InstTinyRAM<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : Instruction {
  field bits<64> Inst;

  field bits<64> SoftFail = 0; // Needed by disassembler.

  let Namespace = "TinyRAM";
  let Size = 8;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString   = asmstr;
  let Pattern = pattern;

  let DecoderNamespace = "TinyRAM";

  let Itinerary = itin;
}

class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
  : InstTinyRAM<outs, ins, asmstr, pattern> {
  let isCodeGenOnly       = 1;
  let isPseudo            = 1;
  let hasNoSchedulingInfo = 1;
}

class F_Inst<dag outs,
             dag ins,
             string asmstr,
             list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : InstTinyRAM<outs, ins, asmstr, pattern, itin> {
  bits<5> op;
  bits<1> immMode;
  bits<4> dst;
  bits<4> op1;

  let Inst{63-59} = op;
  let Inst{58} = immMode;
  let Inst{57-54} = dst;
  let Inst{53-50} = op1;
  let Inst{49-32} = 0b000000000000000000;
}

class F_Imm<dag outs,
             dag ins,
             string asmstr,
             list<dag> pattern,
             InstrItinClass itin = NoItinerary>
    : F_Inst<outs, ins, asmstr, pattern, itin> {
  let immMode = 1;
  }

class F_Reg<dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Inst<outs, ins, asmstr, pattern, itin> {
  bits<4> op2;

  let immMode = 0;
  let Inst {31-4} = 0b0000000000000000000000000000;
  let Inst {3-0} = op2;
  }

class F_RRR<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Reg<outs, ins, asmstr, pattern, itin> {
    let op = _op;
  }

class F_RRI<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Imm<outs, ins, asmstr, pattern, itin> {
    bits<32> op2;

    let op = _op;
    let Inst {31-0} = op2;
  }

class F_RR0<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Reg<outs, ins, asmstr, pattern, itin> {
  let op = _op;

  let op1 = 0b0000;
  }

class F_RI0<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Imm<outs, ins, asmstr, pattern, itin> {
  bits<32> op2;
  let op = _op;

  let op1 = 0b0000;
  let Inst {31-0} = op2;
  }

class F_0R0<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Reg<outs, ins, asmstr, pattern, itin> {
  let op = _op;

  let dst = 0b0000;
  let op1 = 0b0000;
  }

class F_0I0<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Imm<outs, ins, asmstr, pattern, itin> {
  bits<32> op2;
  let op = _op;

  let dst = 0b0000;
  let op1 = 0b0000;
  let Inst {31-0} = op2;
  }

class F_0RR<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Reg<outs, ins, asmstr, pattern, itin> {
  let op = _op;

  let dst = 0b0000;
  }

class F_0RI<bits<5> _op,
            dag outs,
            dag ins,
            string asmstr,
            list<dag> pattern,
            InstrItinClass itin = NoItinerary>
    : F_Imm<outs, ins, asmstr, pattern, itin> {
  let op = _op;
  bits<32> op2;

  let op = _op;
  let dst = 0b0000;
  let Inst {31-0} = op2;
  }