add_llvm_component_library(LLVMTinyRAMInfo
  TinyRAMTargetInfo.cpp

  LINK_COMPONENTS
  Support

  ADD_TO_COMPONENT
  TinyRAM
  )
