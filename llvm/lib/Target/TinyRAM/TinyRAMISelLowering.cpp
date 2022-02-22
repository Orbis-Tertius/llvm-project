//===-- TinyRAMISelLowering.cpp - TinyRAM DAG lowering implementation -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the TinyRAMTargetLowering class.
//
//===----------------------------------------------------------------------===//

#include "TinyRAMISelLowering.h"
#include "TinyRAMTargetMachine.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/KnownBits.h"
#include <cctype>

using namespace llvm;

#define DEBUG_TYPE "tinyRAM-lower"

#include "TinyRAMGenCallingConv.inc"

namespace llvm {

namespace {

const unsigned StackSlotSize = 4;

struct ArgDataPair {
  SDValue SDV;
  ISD::ArgFlagsTy Flags;
};

class TinyRAMTargetLowering : public TargetLowering {
  const TinyRAMSubtarget &Subtarget;

public:
  // Override TargetLowering methods.
  bool hasAndNot(SDValue X) const override {
    return true;
  }

  explicit TinyRAMTargetLowering(const TargetMachine &TM, const TinyRAMSubtarget &STI)
      : TargetLowering(TM), Subtarget(STI) {
    addRegisterClass(MVT::i32, &TinyRAM::GPRRegClass);

    // Compute derived properties from the register classes
    computeRegisterProperties(Subtarget.getRegisterInfo());

    // Set up special registers.
    setStackPointerRegisterToSaveRestore(TinyRAM::SP);

    setSchedulingPreference(Sched::Source);

    // How we extend i1 boolean values.
    setBooleanContents(ZeroOrOneBooleanContent);
    setBooleanVectorContents(ZeroOrOneBooleanContent);

    setMinFunctionAlignment(Align(8));
    setPrefFunctionAlignment(Align(8));

    setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);
    setOperationAction(ISD::BlockAddress, MVT::i32, Custom);

    setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
    setOperationAction(ISD::BR_CC, MVT::i32, Custom);
    setOperationAction(ISD::SETCC, MVT::i32, Custom);

    setOperationAction(ISD::BR_JT, MVT::Other, Expand);
    setOperationAction(ISD::BRCOND, MVT::Other, Expand);

    setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
    setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i64, Expand);

    setMinimumJumpTableEntries(UINT_MAX);
  }

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override {

    switch (Op.getOpcode()) {
    default:
      llvm_unreachable("Don't know how to custom lower this!");
    case ISD::GlobalAddress:
      return LowerGlobalAddress(Op, DAG);
    case ISD::BlockAddress:
      return LowerBlockAddress(Op, DAG);
    case ISD::BR_CC:
      return LowerBR_CC(Op, DAG);
    case ISD::SELECT_CC:
      return LowerSELECT_CC(Op, DAG);
    case ISD::SETCC:
      return LowerSETCC(Op, DAG);
    }

    return SDValue();
  }

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override {
    if (DCI.isBeforeLegalizeOps())
      return SDValue();

    return SDValue();
  }

  SDValue LowerFormalArguments(
      SDValue Chain,
      CallingConv::ID CallConv,
      bool IsVarArg,
      const SmallVectorImpl<ISD::InputArg> &Ins,
      const SDLoc &DL,
      SelectionDAG &DAG,
      SmallVectorImpl<SDValue> &InVals) const override {
    switch (CallConv) {
    default:
      report_fatal_error("Unsupported calling convention");
    case CallingConv::C:
    case CallingConv::Fast:
      return lowerFormalArguments(Chain, CallConv, IsVarArg, Ins, DL, DAG, InVals);
    }
  }

  static SDValue fromVirtualRegister(
      const CCValAssign &VA,
      const SDValue &Chain,
      const SDLoc &Dl,
      SelectionDAG &DAG,
      MachineRegisterInfo &RegInfo) {
    assert(VA.isRegLoc());

    EVT RegVT = VA.getLocVT();
    switch (RegVT.getSimpleVT().SimpleTy) {
    default:
      llvm_unreachable("LowerFormalArguments: unhandled argument type");

    case MVT::i32:
      Register VReg = RegInfo.createVirtualRegister(&TinyRAM::GPRRegClass);
      RegInfo.addLiveIn(VA.getLocReg(), VReg);
      return DAG.getCopyFromReg(Chain, Dl, VReg, RegVT);
    }
  }

  static SDValue fromStackSlot(
      const CCValAssign &VA,
      const SDValue &Chain,
      const SDLoc &Dl,
      SelectionDAG &DAG,
      MachineRegisterInfo &RegInfo,
      MachineFunction &MF,
      MachineFrameInfo &MFI) {
    assert(VA.isMemLoc());
    unsigned ObjSize = VA.getLocVT().getSizeInBits() / 8;
    if (ObjSize > StackSlotSize) {
      llvm_unreachable("LowerFormalArguments: unhandled argument size");
    }
    // Create the frame index object for this incoming parameter...
    int FI = MFI.CreateFixedObject(ObjSize, VA.getLocMemOffset(), true);

    // Create the SelectionDAG nodes corresponding to a load from this parameter
    SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
    return DAG.getLoad(VA.getLocVT(), Dl, Chain, FIN, MachinePointerInfo::getFixedStack(MF, FI));
  }

  static SDValue chainIntoTokenFactor(
      const SDLoc &Dl,
      const SDValue &OldChain,
      const SmallVector<SDValue, 4> &Nodes,
      SelectionDAG &DAG) {
    if (!Nodes.empty()) {
      return DAG.getNode(ISD::TokenFactor, Dl, MVT::Other, Nodes);
    }
    return OldChain;
  }

  struct CopyByValueRet {
    SDValue Memcpy;
    SDValue FrameIndex;
  };

  static CopyByValueRet copyByValue(
      const ArgDataPair &ArgDI,
      SelectionDAG &DAG,
      MachineFrameInfo &MFI,
      const SDValue &Chain,
      const SDLoc &Dl) {
    unsigned Size = ArgDI.Flags.getByValSize();
    Align Alignment = std::max(Align(StackSlotSize), ArgDI.Flags.getNonZeroByValAlign());
    // Create a new object on the stack and copy the pointee into it.
    int FI = MFI.CreateStackObject(Size, Alignment, false);
    SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
    auto Memcpy = DAG.getMemcpy(
        Chain,
        Dl,
        FIN,
        ArgDI.SDV,
        DAG.getConstant(Size, Dl, MVT::i32),
        Alignment,
        false,
        false,
        false,
        MachinePointerInfo(),
        MachinePointerInfo());

    return CopyByValueRet{Memcpy, FIN};
  };

  SDValue lowerFormalArguments(
      const SDValue Chain,
      const CallingConv::ID CallConv,
      bool IsVarArg,
      const SmallVectorImpl<ISD::InputArg> &Ins,
      const SDLoc &Dl,
      SelectionDAG &DAG,
      SmallVectorImpl<SDValue> &InVals) const {
    MachineFunction &MF = DAG.getMachineFunction();
    MachineFrameInfo &MFI = MF.getFrameInfo();
    MachineRegisterInfo &RegInfo = MF.getRegInfo();

    // Assign locations to all of the incoming arguments.
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeFormalArguments(Ins, CC_TinyRAM);

    // All getCopyFromReg ops must precede any getMemcpys
    // to prevent the scheduler clobbering a register before it has been copied.
    SmallVector<SDValue, 4> CFRegNode;
    SmallVector<ArgDataPair, 4> ArgData;

    // 1a. CopyFromReg and Load args
    for (unsigned I = 0, E = ArgLocs.size(); I != E; ++I) {

      CCValAssign &VA = ArgLocs[I];
      SDValue ArgIn;

      if (VA.isRegLoc()) {
        ArgIn = fromVirtualRegister(VA, Chain, Dl, DAG, RegInfo);
        CFRegNode.push_back(ArgIn.getValue(ArgIn->getNumValues() - 1));
      } else {
        assert(VA.isMemLoc());
        ArgIn = fromStackSlot(VA, Chain, Dl, DAG, RegInfo, MF, MFI);
      }
      const ArgDataPair ADP = {ArgIn, Ins[I].Flags};
      ArgData.push_back(ADP);
    }

    // 1b. Varargs
    if (IsVarArg) {
      llvm_unreachable("LowerFormalArguments: varargs not implemented");
    }

    // 2. Chain CopyFromReg nodes into a TokenFactor.
    const auto Chain2 = chainIntoTokenFactor(Dl, Chain, CFRegNode, DAG);

    // 3. Memcpy 'byVal' args & push final InVals.
    // Aggregates passed "byVal" need to be copied by the callee.
    // The callee will use a pointer to this copy, rather than the original pointer.
    SmallVector<SDValue, 4> MemOps;
    for (auto ArgDI = ArgData.begin(), ArgDE = ArgData.end(); ArgDI != ArgDE; ++ArgDI) {
      if (ArgDI->Flags.isByVal() && ArgDI->Flags.getByValSize()) {
        const auto Ret = copyByValue(*ArgDI, DAG, MFI, Chain2, Dl);

        MemOps.push_back(Ret.Memcpy);
        InVals.push_back(Ret.FrameIndex);
      } else {
        InVals.push_back(ArgDI->SDV);
      }
    }

    // 4. Chain mem ops nodes
    const auto Chain3 = chainIntoTokenFactor(Dl, Chain2, MemOps, DAG);

    return Chain3;
  }

  SDValue LowerReturn(
      SDValue Chain,
      CallingConv::ID CallConv,
      bool IsVarArg,
      const SmallVectorImpl<ISD::OutputArg> &Outs,
      const SmallVectorImpl<SDValue> &OutVals,
      const SDLoc &DL,
      SelectionDAG &DAG) const override {

    MachineFunction &MF = DAG.getMachineFunction();

    // Assign locations to each returned value.
    SmallVector<CCValAssign, 16> RetLocs;
    CCState RetCCInfo(CallConv, IsVarArg, MF, RetLocs, *DAG.getContext());
    RetCCInfo.AnalyzeReturn(Outs, RetCC_TinyRAM);

    // Quick exit for void returns
    if (RetLocs.empty())
      return DAG.getNode(TinyRAMISD::RET_FLAG, DL, MVT::Other, Chain);

    SDValue Glue;
    SmallVector<SDValue, 4> RetOps;
    RetOps.push_back(Chain);
    for (unsigned I = 0, E = RetLocs.size(); I != E; ++I) {
      CCValAssign &VA = RetLocs[I];
      SDValue RetValue = OutVals[I];

      // Make the return register live on exit.
      assert(VA.isRegLoc() && "Can only return in registers!");

      // Promote the value as required.
      switch (VA.getLocInfo()) {
      case CCValAssign::SExt:
        RetValue = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), RetValue);
        break;
      case CCValAssign::ZExt:
        RetValue = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), RetValue);
        break;
      case CCValAssign::AExt:
        RetValue = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), RetValue);
        break;
      case CCValAssign::Full:
        break;
      default:
        llvm_unreachable("Unhandled VA.getLocInfo()");
      }

      // Chain and glue the copies together.
      Register Reg = VA.getLocReg();
      Chain = DAG.getCopyToReg(Chain, DL, Reg, RetValue, Glue);
      Glue = Chain.getValue(1);
      RetOps.push_back(DAG.getRegister(Reg, VA.getLocVT()));
    }

    // Update chain and glue.
    RetOps[0] = Chain;
    if (Glue.getNode())
      RetOps.push_back(Glue);

    return DAG.getNode(TinyRAMISD::RET_FLAG, DL, MVT::Other, RetOps);
  }

  SDValue LowerCall(CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const override {
    SelectionDAG &DAG = CLI.DAG;
    SDLoc &Dl = CLI.DL;
    SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
    SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
    SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
    SDValue Chain = CLI.Chain;
    SDValue Callee = CLI.Callee;
    bool &IsTailCall = CLI.IsTailCall;
    CallingConv::ID CallConv = CLI.CallConv;
    bool IsVarArg = CLI.IsVarArg;

    IsTailCall = false;

    switch (CallConv) {
    default:
      report_fatal_error("Unsupported calling convention");
    case CallingConv::Fast:
    case CallingConv::C:
      return LowerCCCCallTo(Chain, Callee, CallConv, IsVarArg, IsTailCall, Outs, OutVals, Ins, Dl, DAG, InVals);
    }
  }

  bool CanLowerReturn(
      CallingConv::ID CallConv,
      MachineFunction &MF,
      bool IsVarArg,
      const SmallVectorImpl<ISD::OutputArg> &Outs,
      LLVMContext &Context) const override {
    assert(IsVarArg == false && "VarArg handling not implemented");

    SmallVector<CCValAssign, 16> RVLocs;
    CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
    if (!CCInfo.CheckReturn(Outs, RetCC_TinyRAM))
      return false;
    if (CCInfo.getNextStackOffset() != 0 && IsVarArg)
      return false; // TODO: understand the condition
    return true;
  }

  /// Lower the result values of a call into the appropriate copies
  /// out of appropriate physical registers / memory locations.
  static SDValue LowerCallResult(
      SDValue Chain,
      SDValue InFlag,
      const SmallVectorImpl<CCValAssign> &RVLocs,
      const SDLoc &Dl,
      SelectionDAG &DAG,
      SmallVectorImpl<SDValue> &InVals) {
    // Copy results out of physical registers.
    for (unsigned I = 0, E = RVLocs.size(); I != E; ++I) {
      const CCValAssign &VA = RVLocs[I];

      assert(VA.isRegLoc()); // returning only via registers

      Chain = DAG.getCopyFromReg(Chain, Dl, VA.getLocReg(), VA.getValVT(), InFlag).getValue(1);
      InFlag = Chain.getValue(2);
      InVals.push_back(Chain.getValue(0));
    }

    return Chain;
  }

  /// Function arguments are copied from virtual regs to (physical regs)/(stack frame),
  /// CALLSEQ_START and CALLSEQ_END are emitted.
  SDValue LowerCCCCallTo(
      SDValue Chain,
      SDValue Callee,
      CallingConv::ID CallConv,
      bool IsVarArg,
      bool IsTailCall,
      const SmallVectorImpl<ISD::OutputArg> &Outs,
      const SmallVectorImpl<SDValue> &OutVals,
      const SmallVectorImpl<ISD::InputArg> &Ins,
      const SDLoc &Dl,
      SelectionDAG &DAG,
      SmallVectorImpl<SDValue> &InVals) const {

    // Analyze operands of the call, assigning locations to each operand.
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs, *DAG.getContext());

    CCInfo.AnalyzeCallOperands(Outs, CC_TinyRAM);

    // return values are placed on registers - never on the stack

    // Get a count of how many bytes are to be pushed on the stack.
    unsigned NumBytes = CCInfo.getNextStackOffset();
    auto PtrVT = getPointerTy(DAG.getDataLayout());

    Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, Dl);

    SmallVector<std::pair<unsigned, SDValue>, 4> RegsToPass;
    SmallVector<SDValue, 12> MemOpChains;

    // Walk the register/memloc assignments, inserting copies/loads.
    for (unsigned I = 0, E = ArgLocs.size(); I != E; ++I) {
      CCValAssign &VA = ArgLocs[I];
      SDValue Arg = OutVals[I];

      // Promote the value if needed.
      switch (VA.getLocInfo()) {
      default:
        llvm_unreachable("Unknown loc info!");
      case CCValAssign::Full:
        break;
      case CCValAssign::SExt:
        Arg = DAG.getNode(ISD::SIGN_EXTEND, Dl, VA.getLocVT(), Arg);
        break;
      case CCValAssign::ZExt:
        Arg = DAG.getNode(ISD::ZERO_EXTEND, Dl, VA.getLocVT(), Arg);
        break;
      case CCValAssign::AExt:
        Arg = DAG.getNode(ISD::ANY_EXTEND, Dl, VA.getLocVT(), Arg);
        break;
      }

      // Arguments that can be passed on register must be kept at RegsToPass vector
      if (VA.isRegLoc()) {
        RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
      } else {
        assert(VA.isMemLoc());

        int Offset = VA.getLocMemOffset();
        assert(Offset % 4 == 0);

        MemOpChains.push_back(
            DAG.getNode(TinyRAMISD::STWSP, Dl, MVT::Other, Chain, Arg, DAG.getConstant(Offset, Dl, MVT::i32)));
      }
    }

    // Transform all store nodes into one single node because
    // all store nodes are independent of each other.
    if (!MemOpChains.empty())
      Chain = DAG.getNode(ISD::TokenFactor, Dl, MVT::Other, MemOpChains);

    // Build a sequence of copy-to-reg nodes chained together with token
    // chain and flag operands which copy the outgoing args into registers.
    // The InFlag in necessary since all emitted instructions must be
    // stuck together.
    SDValue InFlag;
    for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I) {
      Chain = DAG.getCopyToReg(Chain, Dl, RegsToPass[I].first, RegsToPass[I].second, InFlag);
      InFlag = Chain.getValue(1);
    }

    // If the callee is a GlobalAddress node (quite common, every direct call is)
    // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
    // Likewise ExternalSymbol -> TargetExternalSymbol.
    if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
      Callee = DAG.getTargetGlobalAddress(G->getGlobal(), Dl, MVT::i32);
    else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee))
      Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);

    // Returns a chain & a flag for retval copy to use.
    SmallVector<SDValue, 8> Ops;
    Ops.push_back(Chain);
    Ops.push_back(Callee);

    // Add argument registers to the end of the list so that they are
    // known live into the call.
    for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I)
      Ops.push_back(DAG.getRegister(RegsToPass[I].first, RegsToPass[I].second.getValueType()));

    if (InFlag.getNode())
      Ops.push_back(InFlag);

    SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
    Chain = DAG.getNode(TinyRAMISD::CALL, Dl, NodeTys, Ops);
    InFlag = Chain.getValue(1);

    // Create the CALLSEQ_END node.
    Chain = DAG.getCALLSEQ_END(
        Chain, DAG.getConstant(NumBytes, Dl, PtrVT, true), DAG.getConstant(0, Dl, PtrVT, true), InFlag, Dl);
    InFlag = Chain.getValue(1);

    SmallVector<CCValAssign, 16> RVLocs;
    // Analyze return values to determine the number of bytes of stack required.
    CCState RetCCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs, *DAG.getContext());
    RetCCInfo.AnalyzeCallResult(Ins, RetCC_TinyRAM);

    // Handle result values, copying them out of physregs into vregs that we return.
    return LowerCallResult(Chain, InFlag, RVLocs, Dl, DAG, InVals);
  }

  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const {
    auto DL = DAG.getDataLayout();

    const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
    int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();

    // Create the TargetGlobalAddress node, folding in the constant offset.
    SDValue Result = DAG.getTargetGlobalAddress(GV, SDLoc(Op), getPointerTy(DL), Offset);
    return DAG.getNode(TinyRAMISD::WRAPPER, SDLoc(Op), getPointerTy(DL), Result);
  }

  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const {
    SDLoc DL(Op);
    auto PtrVT = getPointerTy(DAG.getDataLayout());
    const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();
    return DAG.getTargetBlockAddress(BA, PtrVT);
  }

  static llvm::SDValue getComparisonSDValue(SDLoc Dl, SelectionDAG &DAG, ISD::CondCode CC, SDValue LHS, SDValue RHS) {
    switch (CC) {
      // signed

    case ISD::CondCode::SETGT:
      return DAG.getNode(TinyRAMISD::CMPG, Dl, MVT::Glue, LHS, RHS);

    case ISD::CondCode::SETGE:
      return DAG.getNode(TinyRAMISD::CMPGE, Dl, MVT::Glue, LHS, RHS);

    case ISD::CondCode::SETLT:
      return DAG.getNode(TinyRAMISD::CMPG, Dl, MVT::Glue, RHS, LHS);

    case ISD::CondCode::SETLE:
      return DAG.getNode(TinyRAMISD::CMPGE, Dl, MVT::Glue, RHS, LHS);

      // unsigned

    case ISD::CondCode::SETUGT:
      return DAG.getNode(TinyRAMISD::CMPA, Dl, MVT::Glue, LHS, RHS);

    case ISD::CondCode::SETUGE:
      return DAG.getNode(TinyRAMISD::CMPAE, Dl, MVT::Glue, LHS, RHS);

    case ISD::CondCode::SETULT:
      return DAG.getNode(TinyRAMISD::CMPA, Dl, MVT::Glue, RHS, LHS);

    case ISD::CondCode::SETULE:
      return DAG.getNode(TinyRAMISD::CMPAE, Dl, MVT::Glue, RHS, LHS);

      // equality
    case ISD::CondCode::SETEQ:
      return DAG.getNode(TinyRAMISD::CMPE, Dl, MVT::Glue, LHS, RHS);

    case ISD::CondCode::SETNE:
      return DAG.getNode(TinyRAMISD::CMPNE, Dl, MVT::Glue, LHS, RHS);

    default:
      llvm_unreachable("Invalid comparison type");
    }

    return llvm::SDValue();
  }

  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
    SDValue Chain = Op.getOperand(0);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
    SDValue LHS = Op.getOperand(2);
    SDValue RHS = Op.getOperand(3);
    SDValue Dest = Op.getOperand(4);
    SDLoc Dl(Op);

    assert(LHS.getSimpleValueType() == MVT::i32);
    assert(RHS.getSimpleValueType() == MVT::i32);

    auto Cmp = getComparisonSDValue(Dl, DAG, CC, LHS, RHS);

    return DAG.getNode(TinyRAMISD::BRCOND, Dl, MVT::Other, Chain, Dest, Cmp);
  }

  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const {
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    SDValue TrueV = Op.getOperand(2);
    SDValue FalseV = Op.getOperand(3);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
    SDLoc Dl(Op);
    assert(LHS.getSimpleValueType() == MVT::i32);
    assert(RHS.getSimpleValueType() == MVT::i32);

    auto Cmp = getComparisonSDValue(Dl, DAG, CC, LHS, RHS);

    SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
    SDValue Ops[] = {TrueV, FalseV, Cmp};

    return DAG.getNode(TinyRAMISD::SELECT_CC, Dl, VTs, Ops);

    return Op;
  }

  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
    SDLoc Dl(Op);

    auto Cmp = getComparisonSDValue(Dl, DAG, CC, LHS, RHS);

    SDValue TrueV = DAG.getConstant(1, Dl, Op.getValueType());
    SDValue FalseV = DAG.getConstant(0, Dl, Op.getValueType());
    SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
    SDValue Ops[] = {TrueV, FalseV, Cmp};

    return DAG.getNode(TinyRAMISD::SELECT_CC, Dl, VTs, Ops);
  }

  const char *getTargetNodeName(unsigned Opcode) const override {
    switch (Opcode) {
#define OPCODE(Opc)                                                                                                    \
  case Opc:                                                                                                            \
    return #Opc
      OPCODE(TinyRAMISD::RET_FLAG);
      OPCODE(TinyRAMISD::CALL);
      OPCODE(TinyRAMISD::STWSP);
      OPCODE(TinyRAMISD::WRAPPER);
      OPCODE(TinyRAMISD::CMPE);
      OPCODE(TinyRAMISD::CMPA);
      OPCODE(TinyRAMISD::CMPAE);
      OPCODE(TinyRAMISD::CMPG);
      OPCODE(TinyRAMISD::CMPGE);
      OPCODE(TinyRAMISD::BRCOND);
#undef OPCODE
    default:
      return nullptr;
    }
  }
};

} // namespace

std::unique_ptr<TargetLowering> createTargetLowering(const TargetMachine &TM, const TinyRAMSubtarget &STI) {
  return std::make_unique<TinyRAMTargetLowering>(TM, STI);
}

} // namespace llvm
