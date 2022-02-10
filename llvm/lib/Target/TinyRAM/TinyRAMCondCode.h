//===-- TinyRAMCondCode.h - TinyRAM Condition Code definition ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that TinyRAM uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYRAM_TINYRAMCONDCODE_H
#define LLVM_LIB_TARGET_TINYRAM_TINYRAMCONDCODE_H

namespace llvm {

namespace TinyRAMCC {
enum CondCode : unsigned {
  EQ = (1 << 2),  // Equal
  NE = (1 << 3),  // Not equal
  GT = (1 << 4),  // Signed greater than
  LE = (1 << 5),  // Signed less than or equal
  LT = (1 << 6),  // Signed less than
  GE = (1 << 7),  // Signed greater than or equal
  HI = (1 << 8),  // Unsigned greater than
  LS = (1 << 9),  // Unsigned less than or equal
  LO = (1 << 10), // Unsigned less than
  HS = (1 << 11), // Unsigned greater than or equal
  BE = (1 << 12), // Byte equal
  NB = (1 << 13), // No byte equal
  HE = (1 << 14), // Half-word equal
  NH = (1 << 15), // No half-word equal
};
} // end namespace TinyRAMCC

} // end namespace llvm

#endif
