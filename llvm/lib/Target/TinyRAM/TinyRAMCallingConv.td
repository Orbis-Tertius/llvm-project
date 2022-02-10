//===-- TinyRAMCallingConv.td - TinyRAM Calling Conventions ------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the TinyRAM architectures.
// Only the OpenBSD variant is supported.
//
//===----------------------------------------------------------------------===//

// TinyRAM 32-bit ELF C Calling convention.
def CC_TinyRAM : CallingConv<[
  // Promote i8/i16 args to i32.
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,

  // Register R12 is used to pass structure return pointer.
  // CCIfSRet<CCIfType<[i32], CCAssignToReg<[R12]>>>,

  // Registers R2 to R4 are used for passing parameters.
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

// TinyRAM 32-bit ELF C return-value convention.
def RetCC_TinyRAM : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3]>>
]>;

// TinyRAM 32-bit ELF C callee saved registers.
// save all but SP
def CSR_TinyRAM : CalleeSavedRegs<(add LR, (sequence "R%d", 4, 11))>;
