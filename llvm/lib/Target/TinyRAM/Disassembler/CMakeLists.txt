add_llvm_component_library(LLVMTinyRAMDisassembler
  TinyRAMDisassembler.cpp

  LINK_COMPONENTS
  MC
  MCDisassembler
  Support
  TinyRAMDesc
  TinyRAMInfo

  ADD_TO_COMPONENT
  TinyRAM
  )
