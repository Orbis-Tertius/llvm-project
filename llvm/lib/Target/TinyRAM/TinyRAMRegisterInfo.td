//===-- TinyRAMRegisterInfo.td - TinyRAM Register definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class TinyRAMReg<bits<5> Enc, string n> : Register<n> {
  let HWEncoding{15-5} = 0;
  let HWEncoding{4-0} = Enc;
  let Namespace = "TinyRAM";
}

// General register
class GRi<bits<5> Enc, string n> : TinyRAMReg<Enc, n>;

// General-purpose registers
foreach I = 0-12 in {
  def R#I : GRi<I, "r"#I>;
}

def FP : GRi<13, "fp">;
def LR : GRi<14, "lr">;
def SP : GRi<15, "sp">;

def GPR : RegisterClass<"TinyRAM", [i32], 32,
                        (add (sequence "R%u", 0, 12), FP, LR, SP)>;

def GPROpnd : RegisterOperand<GPR>;
