/*
 * InstructionDecode.cpp
 *
 * Turn an Instruction into a DecodedInstruction.
 *
 * Mix-ins are added in pipeline order. e.g. From inner to outer:
 *  * Base instruction
 *  * Instruction format (what fields there are, etc.)
 *  * How operands are used (is a register a source or destination?)
 *  * Computation
 *  * Anything which uses the result (e.g. network access)
 *
 *  Created on: 9 Aug 2019
 *      Author: db434
 */

#include "InstructionDecode.h"

#include "InstructionAdapter.h"
#include "InstructionBase.h"
#include "Mixins/InstructionFormats.h"
#include "Mixins/Miscellaneous.h"
#include "Mixins/NetworkAccess.h"
#include "Mixins/OperandSource.h"
#include "Mixins/StructureAccess.h"
#include "Operations/ALU.h"
#include "Operations/ControlFlow.h"
#include "Operations/Memory.h"
#include "Operations/Miscellaneous.h"
#include "Operations/Network.h"

namespace ISA {

// Shorthand for instruction formats.
// Also add in channel map table reading to formats which allow it.
// Can't do the same for registers because we don't yet know whether each
// register is used as a source or destination.
typedef FetchFormat<InstructionBase> FF;
typedef PredicatedFetchFormat<InstructionBase> PFF;
typedef ReadCMT<ZeroRegFormat<InstructionBase>> R0;
typedef ZeroRegNoChannelFormat<InstructionBase> R0nc;
typedef ReadCMT<OneRegFormat<InstructionBase>> R1;
typedef OneRegNoChannelFormat<InstructionBase> R1nc;
typedef ReadCMT<TwoRegFormat<InstructionBase>> R2;
typedef TwoRegNoChannelFormat<InstructionBase> R2nc;
typedef ReadCMT<TwoRegShiftFormat<InstructionBase>> R2s;
typedef ReadCMT<ThreeRegFormat<InstructionBase>> R3;


// TODO: Instruction formats are hard-coded here. Would prefer to take them
// from the auto-generated ISA.h.
// Perhaps update the generation script to build up these types itself.
// Not sure that's entirely possible: e.g. psel.fetch shares computation core
// with psel.

// ALU operations.
typedef NetworkSend<Dest2Src<BitwiseNor<R3>>> Nor;
typedef NetworkSend<Dest1Src1Imm<BitwiseNor<R2>>> NorI;
typedef NetworkSend<Dest2Src<BitwiseAnd<R3>>> And;
typedef NetworkSend<Dest1Src1Imm<BitwiseAnd<R2>>> AndI;
typedef NetworkSend<Dest2Src<BitwiseOr<R3>>> Or;
typedef NetworkSend<Dest1Src1Imm<BitwiseOr<R2>>> OrI;
typedef NetworkSend<Dest2Src<BitwiseXor<R3>>> Xor;
typedef NetworkSend<Dest1Src1Imm<BitwiseXor<R2>>> XorI;
typedef NetworkSend<Dest2Src<SetIfEqual<R3>>> SetEq;
typedef NetworkSend<Dest1Src1Imm<SetIfEqual<R2>>> SetEqI;
typedef NetworkSend<Dest2Src<SetIfNotEqual<R3>>> SetNE;
typedef NetworkSend<Dest1Src1Imm<SetIfNotEqual<R2>>> SetNEI;
typedef NetworkSend<Dest2Src<SetIfLessThan<R3>>> SetLT;
typedef NetworkSend<Dest1Src1Imm<SetIfLessThan<R2>>> SetLTI;
typedef NetworkSend<Dest2Src<SetIfLessThanUnsigned<R3>>> SetLTU;
typedef NetworkSend<Dest1Src1Imm<SetIfLessThanUnsigned<R2>>> SetLTUI;
typedef NetworkSend<Dest2Src<SetIfGreaterThanOrEqual<R3>>> SetGTE;
typedef NetworkSend<Dest1Src1Imm<SetIfGreaterThanOrEqual<R2>>> SetGTEI;
typedef NetworkSend<Dest2Src<SetIfGreaterThanOrEqualUnsigned<R3>>> SetGTEU;
typedef NetworkSend<Dest1Src1Imm<SetIfGreaterThanOrEqualUnsigned<R2>>> SetGTEUI;
typedef NetworkSend<Dest2Src<ShiftLeftLogical<R3>>> SLL;
typedef NetworkSend<Dest1Src1Imm<ShiftLeftLogical<R2s>>> SLLI;
typedef NetworkSend<Dest2Src<ShiftRightLogical<R3>>> SRL;
typedef NetworkSend<Dest1Src1Imm<ShiftRightLogical<R2s>>> SRLI;
typedef NetworkSend<Dest2Src<ShiftRightArithmetic<R3>>> SRA;
typedef NetworkSend<Dest1Src1Imm<ShiftRightArithmetic<R2s>>> SRAI;
typedef NetworkSend<Dest2Src<Add<R3>>> AddU;
typedef NetworkSend<Dest1Src1Imm<Add<R2>>> AddUI;
typedef NetworkSend<Dest2Src<Subtract<R3>>> SubU;
typedef NetworkSend<Dest1Src1Imm<Subtract<R2>>> SubUI;

// Other compute.
typedef NetworkSend<Dest2Src<PredicatedSelect<R3>>> PSel;
typedef NetworkSend<Dest2Src<MultiplyLowWord<R3>>> MulLW;
typedef NetworkSend<Dest2Src<MultiplyHighWord<R3>>> MulHW;
typedef NetworkSend<Dest2Src<MultiplyHighWordUnsigned<R3>>> MulHWU;
typedef NetworkSend<Dest1Src<CountLeadingZeros<R2>>> CLZ;
typedef Dest1Imm<LoadLowerImmediate<R1nc>> LLI;
typedef Dest1SrcShared1Imm<LoadUpperImmediate<R1nc>> LUI;

// Structure access.
typedef NetworkSend<Dest1Src<ReadScratchpad<R2>>> ScratchRd;
typedef NetworkSend<Dest1Imm<ReadScratchpad<R1>>> ScratchRdI;
typedef NoDest2Src<WriteScratchpad<R2nc>> ScratchWr;
typedef NoDest1Src1Imm<WriteScratchpad<R1nc>> ScratchWrI;
typedef NetworkSend<Dest1Src<GetChannelMap<R2>>> GetChMap;
typedef NetworkSend<Dest1Imm<GetChannelMap<R1>>> GetChMapI;
typedef NoDest2Src<WriteCMT<R2nc>> SetChMap;
typedef NoDest1Src1Imm<WriteCMT<R1nc>> SetChMapI;
typedef NetworkSend<Dest1Imm<ReadCregs<R1>>> CRegRdI;
typedef NoDest1Src1Imm<WriteCregs<R1nc>> CRegWrI;

// Control flow. Fetches don't necessarily send on the network, so don't use
// NetworkSend.
typedef ComputeEarly<InstructionFetch<Relative<NoDest2Imm<PredicatedSelect<PFF>>>>> PSelFetchR;
typedef ComputeEarly<InstructionFetch<NoDest2Src<PredicatedSelect<R2nc>>>> PSelFetch;
typedef ComputeEarly<InstructionFetch<Relative<NoDest1Imm<NoOp<FF>>>>> FetchR;
typedef ComputeEarly<InstructionFetch<NoDest1Src<NoOp<R1nc>>>> Fetch;
typedef ComputeEarly<PersistentFetch<Relative<NoDest1Imm<NoOp<FF>>>>> FetchPstR;
typedef ComputeEarly<PersistentFetch<NoDest1Src<NoOp<R1nc>>>> FetchPst;
typedef ComputeEarly<NoJumpFetch<Relative<NoDest1Imm<NoOp<FF>>>>> FillR;
typedef ComputeEarly<NoJumpFetch<NoDest1Src<NoOp<R1nc>>>> Fill;
typedef ComputeEarly<InBufferJump<NoDest1Imm<NoOp<R0nc>>>> IBJmp;

// Memory access.
typedef MemorySend<NoDest1Src1Imm<LoadWord<R1>>> LdW;
typedef MemorySend<NoDest1Src1Imm<LoadWord<R1>>> LdHWU;
typedef MemorySend<NoDest1Src1Imm<LoadWord<R1>>> LdBU;
typedef MemorySend<NoDest1Src1Imm<LoadLinked<R1>>> LdL;
typedef MemorySend<NoDest2Src1Imm<StoreWord<R2>>> StW;
typedef MemorySend<NoDest2Src1Imm<StoreHalfWord<R2>>> StHW;
typedef MemorySend<NoDest2Src1Imm<StoreByte<R2>>> StB;
typedef MemorySend<NoDest2Src1Imm<StoreConditional<R2>>> StC;
typedef MemorySend<NoDest2Src1Imm<LoadAndAdd<R2>>> LdAdd;
typedef MemorySend<NoDest2Src1Imm<LoadAndOr<R2>>> LdOr;
typedef MemorySend<NoDest2Src1Imm<LoadAndAnd<R2>>> LdAnd;
typedef MemorySend<NoDest2Src1Imm<LoadAndXor<R2>>> LdXor;
typedef MemorySend<NoDest2Src1Imm<Exchange<R2>>> Swap;

// Network management.
typedef ComputeEarly<NoDest1Imm<WaitOnChannelEnd<R0>>> WOChE;
typedef ComputeEarly<Dest1Imm<SelectChannel<R1nc>>> SelCh;
typedef ComputeEarly<Dest1Imm<TestChannel<R1>>> TstChI;

// Miscellaneous.
typedef NetworkSend<RemoteNextIPK<R0>> RmtNxIPK;
typedef ComputeEarly<RemoteExecute<R0>> RmtExecute;
typedef NextIPK<R0nc> NxIPK;
typedef NetworkSend<Dest1Src<IndirectRead<R2>>> IRdR;
typedef NetworkSend<Dest1Src<IndirectWrite<R2>>> IWtR;
typedef NoDest1Imm<SystemCall<R0nc>> SysCall;
typedef NoDest1Src1Imm<SendConfig<R1>> SendConfig;

} // end namespace


// Really short name because this is used a lot.
template<class T>
using A = InstructionAdapter<T>;

DecodedInstruction decodeInstruction(Instruction encoded) {

  opcode_t opcode = encoded.opcode();
  InstructionInterface* instruction;

  switch (opcode) {

    // Plain ALU operations.
    case 0: {
      function_t function = encoded.function();

      switch (function) {
        case ISA::FN_NOR: instruction = new A<ISA::Nor>(encoded); break;
        case ISA::FN_AND: instruction = new A<ISA::And>(encoded); break;
        case ISA::FN_OR: instruction = new A<ISA::Or>(encoded); break;
        case ISA::FN_XOR: instruction = new A<ISA::Xor>(encoded); break;
        case ISA::FN_SETEQ: instruction = new A<ISA::SetEq>(encoded); break;
        case ISA::FN_SETNE: instruction = new A<ISA::SetNE>(encoded); break;
        case ISA::FN_SETLT: instruction = new A<ISA::SetLT>(encoded); break;
        case ISA::FN_SETLTU: instruction = new A<ISA::SetLTU>(encoded); break;
        case ISA::FN_SETGTE: instruction = new A<ISA::SetGTE>(encoded); break;
        case ISA::FN_SETGTEU: instruction = new A<ISA::SetGTEU>(encoded); break;
        case ISA::FN_SLL: instruction = new A<ISA::SLL>(encoded); break;
        case ISA::FN_SRL: instruction = new A<ISA::SRL>(encoded); break;
        case ISA::FN_SRA: instruction = new A<ISA::SRA>(encoded); break;
        case ISA::FN_ADDU: instruction = new A<ISA::AddU>(encoded); break;
        case ISA::FN_SUBU: instruction = new A<ISA::SubU>(encoded); break;
        default: assert(false); instruction = NULL; break;
      }
      break;
    }

    // ALU operations which modify the predicate register.
    case 1: {
      function_t function = encoded.function();

      switch (function) {
        case ISA::FN_NOR: instruction = new A<WritePredicate<ISA::Nor>>(encoded); break;
        case ISA::FN_AND: instruction = new A<WritePredicate<ISA::And>>(encoded); break;
        case ISA::FN_OR: instruction = new A<WritePredicate<ISA::Or>>(encoded); break;
        case ISA::FN_XOR: instruction = new A<WritePredicate<ISA::Xor>>(encoded); break;
        case ISA::FN_SETEQ: instruction = new A<WritePredicate<ISA::SetEq>>(encoded); break;
        case ISA::FN_SETNE: instruction = new A<WritePredicate<ISA::SetNE>>(encoded); break;
        case ISA::FN_SETLT: instruction = new A<WritePredicate<ISA::SetLT>>(encoded); break;
        case ISA::FN_SETLTU: instruction = new A<WritePredicate<ISA::SetLTU>>(encoded); break;
        case ISA::FN_SETGTE: instruction = new A<WritePredicate<ISA::SetGTE>>(encoded); break;
        case ISA::FN_SETGTEU: instruction = new A<WritePredicate<ISA::SetGTEU>>(encoded); break;
        case ISA::FN_SRL: instruction = new A<WritePredicate<ISA::SRL>>(encoded); break;
        case ISA::FN_ADDU: instruction = new A<WritePredicate<ISA::AddU>>(encoded); break;
        case ISA::FN_SUBU: instruction = new A<WritePredicate<ISA::SubU>>(encoded); break;
        default: assert(false); instruction = NULL; break;
      }
      break;
    }

    case ISA::OP_NORI: instruction = new A<ISA::NorI>(encoded); break;
    case ISA::OP_NORI_P: instruction = new A<WritePredicate<ISA::NorI>>(encoded); break;
    case ISA::OP_PSEL: instruction = new A<ISA::PSel>(encoded); break;
    case ISA::OP_NXIPK: instruction = new A<ISA::NxIPK>(encoded); break;
    case ISA::OP_ANDI: instruction = new A<ISA::AndI>(encoded); break;
    case ISA::OP_ANDI_P: instruction = new A<WritePredicate<ISA::AndI>>(encoded); break;
    case ISA::OP_MULHW: instruction = new A<ISA::MulHW>(encoded); break;
    case ISA::OP_ORI: instruction = new A<ISA::OrI>(encoded); break;
    case ISA::OP_ORI_P: instruction = new A<WritePredicate<ISA::OrI>>(encoded); break;
    case ISA::OP_MULLW: instruction = new A<ISA::MulLW>(encoded); break;
    case ISA::OP_XORI: instruction = new A<ISA::XorI>(encoded); break;
    case ISA::OP_XORI_P: instruction = new A<WritePredicate<ISA::XorI>>(encoded); break;
    case ISA::OP_MULHWU: instruction = new A<ISA::MulHWU>(encoded); break;
    case ISA::OP_SETEQI: instruction = new A<ISA::SetEqI>(encoded); break;
    case ISA::OP_SETEQI_P: instruction = new A<WritePredicate<ISA::SetEqI>>(encoded); break;
    case ISA::OP_SETNEI: instruction = new A<ISA::SetNEI>(encoded); break;
    case ISA::OP_SETNEI_P: instruction = new A<WritePredicate<ISA::SetNEI>>(encoded); break;
    case ISA::OP_SETLTI: instruction = new A<ISA::SetLTI>(encoded); break;
    case ISA::OP_SETLTI_P: instruction = new A<WritePredicate<ISA::SetLTI>>(encoded); break;
    case ISA::OP_SETLTUI: instruction = new A<ISA::SetLTUI>(encoded); break;
    case ISA::OP_SETLTUI_P: instruction = new A<WritePredicate<ISA::SetLTUI>>(encoded); break;
    case ISA::OP_STC: instruction = new A<ISA::StC>(encoded); break;
    case ISA::OP_SETGTEI: instruction = new A<ISA::SetGTEI>(encoded); break;
    case ISA::OP_SETGTEI_P: instruction = new A<WritePredicate<ISA::SetGTEI>>(encoded); break;
    case ISA::OP_LDADD: instruction = new A<ISA::LdAdd>(encoded); break;
    case ISA::OP_SETGTEUI: instruction = new A<ISA::SetGTEUI>(encoded); break;
    case ISA::OP_SETGTEUI_P: instruction = new A<WritePredicate<ISA::SetGTEUI>>(encoded); break;
    case ISA::OP_LDOR: instruction = new A<ISA::LdOr>(encoded); break;
    case ISA::OP_SLLI: instruction = new A<ISA::SLLI>(encoded); break;
    case ISA::OP_LDAND: instruction = new A<ISA::LdAnd>(encoded); break;
    case ISA::OP_SRLI: instruction = new A<ISA::SRLI>(encoded); break;
    case ISA::OP_SRLI_P: instruction = new A<WritePredicate<ISA::SRLI>>(encoded); break;
    case ISA::OP_LDXOR: instruction = new A<ISA::LdXor>(encoded); break;
    case ISA::OP_SRAI: instruction = new A<ISA::SRAI>(encoded); break;
    case ISA::OP_EXCHANGE: instruction = new A<ISA::Swap>(encoded); break;
    case ISA::OP_ADDUI: instruction = new A<ISA::AddUI>(encoded); break;
    case ISA::OP_ADDUI_P: instruction = new A<WritePredicate<ISA::AddUI>>(encoded); break;
    case ISA::OP_CLZ: instruction = new A<ISA::CLZ>(encoded); break;
    case ISA::OP_IWTR: instruction = new A<ISA::IWtR>(encoded); break;
    case ISA::OP_RMTNXIPK: instruction = new A<ISA::RmtNxIPK>(encoded); break;
    case ISA::OP_LDW: instruction = new A<ISA::LdW>(encoded); break;
    case ISA::OP_LDL: instruction = new A<ISA::LdL>(encoded); break;
    case ISA::OP_PSEL_FETCH: instruction = new A<ISA::PSelFetch>(encoded); break;
    case ISA::OP_RMTEXECUTE: instruction = new A<ISA::RmtExecute>(encoded); break;
    case ISA::OP_LDHWU: instruction = new A<ISA::LdHWU>(encoded); break;
    case ISA::OP_SENDCONFIG: instruction = new A<ISA::SendConfig>(encoded); break;
    case ISA::OP_STW: instruction = new A<ISA::StW>(encoded); break;
    case ISA::OP_SYSCALL: instruction = new A<ISA::SysCall>(encoded); break;
    case ISA::OP_LDBU: instruction = new A<ISA::LdBU>(encoded); break;
    case ISA::OP_SCRATCHWR: instruction = new A<ISA::ScratchWr>(encoded); break;
    case ISA::OP_IBJMP: instruction = new A<ISA::IBJmp>(encoded); break;
    case ISA::OP_SCRATCHWRI: instruction = new A<ISA::ScratchWrI>(encoded); break;
    case ISA::OP_SETCHMAP: instruction = new A<ISA::SetChMap>(encoded); break;
    case ISA::OP_WOCHE: instruction = new A<ISA::WOChE>(encoded); break;
    case ISA::OP_SETCHMAPI: instruction = new A<ISA::SetChMapI>(encoded); break;
    case ISA::OP_STHW: instruction = new A<ISA::StHW>(encoded); break;
    case ISA::OP_FETCHR: instruction = new A<ISA::FetchR>(encoded); break;
    case ISA::OP_FETCH: instruction = new A<ISA::Fetch>(encoded); break;
    case ISA::OP_STB: instruction = new A<ISA::StB>(encoded); break;
    case ISA::OP_FILLR: instruction = new A<ISA::FillR>(encoded); break;
    case ISA::OP_FETCHPST: instruction = new A<ISA::FetchPst>(encoded); break;
    case ISA::OP_LUI: instruction = new A<ISA::LUI>(encoded); break;
    case ISA::OP_FETCHPSTR: instruction = new A<ISA::FetchPstR>(encoded); break;
    case ISA::OP_FILL: instruction = new A<ISA::Fill>(encoded); break;
    case ISA::OP_CREGWRI: instruction = new A<ISA::CRegWrI>(encoded); break;
    case ISA::OP_PSEL_FETCHR: instruction = new A<ISA::PSelFetchR>(encoded); break;
    case ISA::OP_SELCH: instruction = new A<ISA::SelCh>(encoded); break;
    case ISA::OP_IRDR: instruction = new A<ISA::IRdR>(encoded); break;
    case ISA::OP_CREGRDI: instruction = new A<ISA::CRegRdI>(encoded); break;
    case ISA::OP_TSTCHI: instruction = new A<ISA::TstChI>(encoded); break;
    case ISA::OP_TSTCHI_P: instruction = new A<WritePredicate<ISA::TstChI>>(encoded); break;
    case ISA::OP_GETCHMAPI: instruction = new A<ISA::GetChMapI>(encoded); break;
    case ISA::OP_GETCHMAP: instruction = new A<ISA::GetChMap>(encoded); break;
    case ISA::OP_LLI: instruction = new A<ISA::LLI>(encoded); break;
    case ISA::OP_SCRATCHRDI: instruction = new A<ISA::ScratchRdI>(encoded); break;
    case ISA::OP_SCRATCHRD: instruction = new A<ISA::ScratchRd>(encoded); break;

    default:
      assert(false); instruction = NULL; break;
  }

  return DecodedInstruction(instruction);
}
