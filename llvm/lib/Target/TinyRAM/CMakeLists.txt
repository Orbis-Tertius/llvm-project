add_llvm_component_group(TinyRAM)

set(LLVM_TARGET_DEFINITIONS TinyRAM.td)

tablegen(LLVM TinyRAMGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM TinyRAMGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM TinyRAMGenCallingConv.inc -gen-callingconv)
tablegen(LLVM TinyRAMGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM TinyRAMGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM TinyRAMGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM TinyRAMGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM TinyRAMGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM TinyRAMGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(TinyRAMCommonTableGen)

add_llvm_target(TinyRAMCodeGen
  TinyRAMAsmPrinter.cpp
  TinyRAMFrameLowering.cpp
  TinyRAMISelDAGToDAG.cpp
  TinyRAMISelLowering.cpp
  TinyRAMInstrInfo.cpp
#  TinyRAMMachineFunctionInfo.cpp
  TinyRAMMCInstLower.cpp
  TinyRAMRegisterInfo.cpp
  TinyRAMSubtarget.cpp
  TinyRAMTargetMachine.cpp
  )

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
