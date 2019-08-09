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
typedef NetworkSend<BitwiseNor<Dest2Src<R3>>> Nor;
typedef NetworkSend<BitwiseNor<Dest1Src1Imm<R2>>> NorI;
typedef NetworkSend<BitwiseAnd<Dest2Src<R3>>> And;
typedef NetworkSend<BitwiseAnd<Dest1Src1Imm<R2>>> AndI;
typedef NetworkSend<BitwiseOr<Dest2Src<R3>>> Or;
typedef NetworkSend<BitwiseOr<Dest1Src1Imm<R2>>> OrI;
typedef NetworkSend<BitwiseXor<Dest2Src<R3>>> Xor;
typedef NetworkSend<BitwiseXor<Dest1Src1Imm<R2>>> XorI;
typedef NetworkSend<SetIfEqual<Dest2Src<R3>>> SetEq;
typedef NetworkSend<SetIfEqual<Dest1Src1Imm<R2>>> SetEqI;
typedef NetworkSend<SetIfNotEqual<Dest2Src<R3>>> SetNE;
typedef NetworkSend<SetIfNotEqual<Dest1Src1Imm<R2>>> SetNEI;
typedef NetworkSend<SetIfLessThan<Dest2Src<R3>>> SetLT;
typedef NetworkSend<SetIfLessThan<Dest1Src1Imm<R2>>> SetLTI;
typedef NetworkSend<SetIfLessThanUnsigned<Dest2Src<R3>>> SetLTU;
typedef NetworkSend<SetIfLessThanUnsigned<Dest1Src1Imm<R2>>> SetLTUI;
typedef NetworkSend<SetIfGreaterThanOrEqual<Dest2Src<R3>>> SetGTE;
typedef NetworkSend<SetIfGreaterThanOrEqual<Dest1Src1Imm<R2>>> SetGTEI;
typedef NetworkSend<SetIfGreaterThanOrEqualUnsigned<Dest2Src<R3>>> SetGTEU;
typedef NetworkSend<SetIfGreaterThanOrEqualUnsigned<Dest1Src1Imm<R2>>> SetGTEUI;
typedef NetworkSend<ShiftLeftLogical<Dest2Src<R3>>> SLL;
typedef NetworkSend<ShiftLeftLogical<Dest1Src1Imm<R2s>>> SLLI;
typedef NetworkSend<ShiftRightLogical<Dest2Src<R3>>> SRL;
typedef NetworkSend<ShiftRightLogical<Dest1Src1Imm<R2s>>> SRLI;
typedef NetworkSend<ShiftRightArithmetic<Dest2Src<R3>>> SRA;
typedef NetworkSend<ShiftRightArithmetic<Dest1Src1Imm<R2s>>> SRAI;
typedef NetworkSend<Add<Dest2Src<R3>>> AddU;
typedef NetworkSend<Add<Dest1Src1Imm<R2>>> AddUI;
typedef NetworkSend<Subtract<Dest2Src<R3>>> SubU;
typedef NetworkSend<Subtract<Dest1Src1Imm<R2>>> SubUI;

// Other compute.
typedef NetworkSend<PredicatedSelect<ReadPredicate<Dest2Src<R3>>>> PSel;
typedef NetworkSend<MultiplyLowWord<Dest2Src<R3>>> MulLW;
typedef NetworkSend<MultiplyHighWord<Dest2Src<R3>>> MulHW;
typedef NetworkSend<MultiplyHighWordUnsigned<Dest2Src<R3>>> MulHWU;
typedef NetworkSend<CountLeadingZeros<Dest1Src<R2>>> CLZ;
typedef LoadLowerImmediate<Dest1Imm<R1nc>> LLI;
typedef LoadUpperImmediate<Dest1SrcShared1Imm<R1nc>> LUI;

// Structure access.
typedef NetworkSend<ReadScratchpad<Dest1Src<R2>>> ScratchRd;
typedef NetworkSend<ReadScratchpad<Dest1Imm<R1>>> ScratchRdI;
typedef WriteScratchpad<NoDest2Src<R2nc>> ScratchWr;
typedef WriteScratchpad<NoDest1Src1Imm<R1nc>> ScratchWrI;
typedef NetworkSend<GetChannelMap<Dest1Src<R2>>> GetChMap;
typedef NetworkSend<GetChannelMap<Dest1Imm<R1>>> GetChMapI;
typedef WriteCMT<NoDest2Src<R2nc>> SetChMap;
typedef WriteCMT<NoDest1Src1Imm<R1nc>> SetChMapI;
typedef NetworkSend<ReadCregs<Dest1Imm<R1>>> CRegRdI;
typedef WriteCregs<NoDest1Src1Imm<R1nc>> CRegWrI;

// Control flow. Fetches don't necessarily send on the network, so don't use
// NetworkSend.
typedef ComputeEarly<InstructionFetch<Relative<PredicatedSelect<ReadPredicate<NoDest2Imm<PFF>>>>>> PSelFetchR;
typedef ComputeEarly<InstructionFetch<PredicatedSelect<ReadPredicate<NoDest2Src<R2nc>>>>> PSelFetch;
typedef ComputeEarly<InstructionFetch<Relative<NoOp<NoDest1Imm<FF>>>>> FetchR;
typedef ComputeEarly<InstructionFetch<NoOp<NoDest1Src<R1nc>>>> Fetch;
typedef ComputeEarly<PersistentFetch<Relative<NoOp<NoDest1Imm<FF>>>>> FetchPstR;
typedef ComputeEarly<PersistentFetch<NoOp<NoDest1Src<R1nc>>>> FetchPst;
typedef ComputeEarly<NoJumpFetch<Relative<NoOp<NoDest1Imm<FF>>>>> FillR;
typedef ComputeEarly<NoJumpFetch<NoOp<NoDest1Src<R1nc>>>> Fill;
typedef ComputeEarly<InBufferJump<NoOp<NoDest1Imm<R0nc>>>> IBJmp;

// Memory access.
typedef MemorySend<LoadWord<NoDest1Src1Imm<R1>>> LdW;
typedef MemorySend<LoadWord<NoDest1Src1Imm<R1>>> LdHWU;
typedef MemorySend<LoadWord<NoDest1Src1Imm<R1>>> LdBU;
typedef MemorySend<LoadLinked<NoDest1Src1Imm<R1>>> LdL;
typedef MemorySend<StoreWord<NoDest2Src1Imm<R2>>> StW;
typedef MemorySend<StoreHalfWord<NoDest2Src1Imm<R2>>> StHW;
typedef MemorySend<StoreByte<NoDest2Src1Imm<R2>>> StB;
typedef MemorySend<StoreConditional<NoDest2Src1Imm<R2>>> StC;
typedef MemorySend<LoadAndAdd<NoDest2Src1Imm<R2>>> LdAdd;
typedef MemorySend<LoadAndOr<NoDest2Src1Imm<R2>>> LdOr;
typedef MemorySend<LoadAndAnd<NoDest2Src1Imm<R2>>> LdAnd;
typedef MemorySend<LoadAndXor<NoDest2Src1Imm<R2>>> LdXor;
typedef MemorySend<Exchange<NoDest2Src1Imm<R2>>> Exchange;

// Network management.
typedef ComputeEarly<WaitOnChannelEnd<NoDest1Imm<R0>>> WOChE;
typedef ComputeEarly<SelectChannel<Dest1Imm<R1nc>>> SelCh;
typedef ComputeEarly<TestChannel<Dest1Imm<R1>>> TstChI;

// Miscellaneous.
typedef NetworkSend<RemoteNextIPK<R0>> RmtNxIPK;
typedef ComputeEarly<RemoteExecute<R0>> RmtExecute;
typedef NextIPK<R0nc> NxIPK;
typedef NetworkSend<IndirectRead<Dest1Src<R2>>> IRdR;
typedef NetworkSend<IndirectWrite<Dest1Src<R2>>> IWtR;
typedef SystemCall<NoDest1Imm<R0nc>> SysCall;
typedef SendConfig<NoDest1Src1Imm<R1>> SendConfig;


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
        case ISA::FN_NOR: instruction = new A<Nor>(encoded); break;
        case ISA::FN_AND: instruction = new A<And>(encoded); break;
        case ISA::FN_OR: instruction = new A<Or>(encoded); break;
        case ISA::FN_XOR: instruction = new A<Xor>(encoded); break;
        case ISA::FN_SETEQ: instruction = new A<SetEq>(encoded); break;
        case ISA::FN_SETNE: instruction = new A<SetNE>(encoded); break;
        case ISA::FN_SETLT: instruction = new A<SetLT>(encoded); break;
        case ISA::FN_SETLTU: instruction = new A<SetLTU>(encoded); break;
        case ISA::FN_SETGTE: instruction = new A<SetGTE>(encoded); break;
        case ISA::FN_SETGTEU: instruction = new A<SetGTEU>(encoded); break;
        case ISA::FN_SLL: instruction = new A<SLL>(encoded); break;
        case ISA::FN_SRL: instruction = new A<SRL>(encoded); break;
        case ISA::FN_SRA: instruction = new A<SRA>(encoded); break;
        case ISA::FN_ADDU: instruction = new A<AddU>(encoded); break;
        case ISA::FN_SUBU: instruction = new A<SubU>(encoded); break;
        default: assert(false); instruction = NULL; break;
      }
      break;
    }

    // ALU operations which modify the predicate register.
    case 1: {
      function_t function = encoded.function();

      switch (function) {
        case ISA::FN_NOR: instruction = new A<WritePredicate<Nor>>(encoded); break;
        case ISA::FN_AND: instruction = new A<WritePredicate<And>>(encoded); break;
        case ISA::FN_OR: instruction = new A<WritePredicate<Or>>(encoded); break;
        case ISA::FN_XOR: instruction = new A<WritePredicate<Xor>>(encoded); break;
        case ISA::FN_SETEQ: instruction = new A<WritePredicate<SetEq>>(encoded); break;
        case ISA::FN_SETNE: instruction = new A<WritePredicate<SetNE>>(encoded); break;
        case ISA::FN_SETLT: instruction = new A<WritePredicate<SetLT>>(encoded); break;
        case ISA::FN_SETLTU: instruction = new A<WritePredicate<SetLTU>>(encoded); break;
        case ISA::FN_SETGTE: instruction = new A<WritePredicate<SetGTE>>(encoded); break;
        case ISA::FN_SETGTEU: instruction = new A<WritePredicate<SetGTEU>>(encoded); break;
        case ISA::FN_SLL: instruction = new A<WritePredicate<SLL>>(encoded); break;
        case ISA::FN_SRL: instruction = new A<WritePredicate<SRL>>(encoded); break;
        case ISA::FN_SRA: instruction = new A<WritePredicate<SRA>>(encoded); break;
        case ISA::FN_ADDU: instruction = new A<WritePredicate<AddU>>(encoded); break;
        case ISA::FN_SUBU: instruction = new A<WritePredicate<SubU>>(encoded); break;
        default: assert(false); instruction = NULL; break;
      }
      break;
    }

    case ISA::OP_NORI: instruction = new A<NorI>(encoded); break;
    case ISA::OP_NORI_P: instruction = new A<WritePredicate<NorI>>(encoded); break;
    case ISA::OP_PSEL: instruction = new A<PSel>(encoded); break;
    case ISA::OP_NXIPK: instruction = new A<NxIPK>(encoded); break;
    case ISA::OP_ANDI: instruction = new A<AndI>(encoded); break;
    case ISA::OP_ANDI_P: instruction = new A<WritePredicate<AndI>>(encoded); break;
    case ISA::OP_MULHW: instruction = new A<MulHW>(encoded); break;
    case ISA::OP_ORI: instruction = new A<OrI>(encoded); break;
    case ISA::OP_ORI_P: instruction = new A<WritePredicate<OrI>>(encoded); break;
    case ISA::OP_MULLW: instruction = new A<MulLW>(encoded); break;
    case ISA::OP_XORI: instruction = new A<XorI>(encoded); break;
    case ISA::OP_XORI_P: instruction = new A<WritePredicate<XorI>>(encoded); break;
    case ISA::OP_MULHWU: instruction = new A<MulHWU>(encoded); break;
    case ISA::OP_SETEQI: instruction = new A<SetEqI>(encoded); break;
    case ISA::OP_SETEQI_P: instruction = new A<WritePredicate<SetEqI>>(encoded); break;
    case ISA::OP_SETNEI: instruction = new A<SetNEI>(encoded); break;
    case ISA::OP_SETNEI_P: instruction = new A<WritePredicate<SetNEI>>(encoded); break;
    case ISA::OP_SETLTI: instruction = new A<SetLTI>(encoded); break;
    case ISA::OP_SETLTI_P: instruction = new A<WritePredicate<SetLTI>>(encoded); break;
    case ISA::OP_SETLTUI: instruction = new A<SetLTUI>(encoded); break;
    case ISA::OP_SETLTUI_P: instruction = new A<WritePredicate<SetLTUI>>(encoded); break;
    case ISA::OP_STC: instruction = new A<StC>(encoded); break;
    case ISA::OP_SETGTEI: instruction = new A<SetGTEI>(encoded); break;
    case ISA::OP_SETGTEI_P: instruction = new A<WritePredicate<SetGTEI>>(encoded); break;
    case ISA::OP_LDADD: instruction = new A<LdAdd>(encoded); break;
    case ISA::OP_SETGTEUI: instruction = new A<SetGTEUI>(encoded); break;
    case ISA::OP_SETGTEUI_P: instruction = new A<WritePredicate<SetGTEUI>>(encoded); break;
    case ISA::OP_LDOR: instruction = new A<LdOr>(encoded); break;
    case ISA::OP_SLLI: instruction = new A<SLLI>(encoded); break;
    case ISA::OP_LDAND: instruction = new A<LdAnd>(encoded); break;
    case ISA::OP_SRLI: instruction = new A<SRLI>(encoded); break;
    case ISA::OP_SRLI_P: instruction = new A<WritePredicate<SRLI>>(encoded); break;
    case ISA::OP_LDXOR: instruction = new A<LdXor>(encoded); break;
    case ISA::OP_SRAI: instruction = new A<SRAI>(encoded); break;
    case ISA::OP_EXCHANGE: instruction = new A<Exchange>(encoded); break;
    case ISA::OP_ADDUI: instruction = new A<AddUI>(encoded); break;
    case ISA::OP_ADDUI_P: instruction = new A<WritePredicate<AddUI>>(encoded); break;
    case ISA::OP_CLZ: instruction = new A<CLZ>(encoded); break;
    case ISA::OP_IWTR: instruction = new A<IWtR>(encoded); break;
    case ISA::OP_RMTNXIPK: instruction = new A<RmtNxIPK>(encoded); break;
    case ISA::OP_LDW: instruction = new A<LdW>(encoded); break;
    case ISA::OP_LDL: instruction = new A<LdL>(encoded); break;
    case ISA::OP_PSEL_FETCH: instruction = new A<PSelFetch>(encoded); break;
    case ISA::OP_RMTEXECUTE: instruction = new A<RmtExecute>(encoded); break;
    case ISA::OP_LDHWU: instruction = new A<LdHWU>(encoded); break;
    case ISA::OP_SENDCONFIG: instruction = new A<SendConfig>(encoded); break;
    case ISA::OP_STW: instruction = new A<StW>(encoded); break;
    case ISA::OP_SYSCALL: instruction = new A<SysCall>(encoded); break;
    case ISA::OP_LDBU: instruction = new A<LdBU>(encoded); break;
    case ISA::OP_SCRATCHWR: instruction = new A<ScratchWr>(encoded); break;
    case ISA::OP_IBJMP: instruction = new A<IBJmp>(encoded); break;
    case ISA::OP_SCRATCHWRI: instruction = new A<ScratchWrI>(encoded); break;
    case ISA::OP_SETCHMAP: instruction = new A<SetChMap>(encoded); break;
    case ISA::OP_WOCHE: instruction = new A<WOChE>(encoded); break;
    case ISA::OP_SETCHMAPI: instruction = new A<SetChMapI>(encoded); break;
    case ISA::OP_STHW: instruction = new A<StHW>(encoded); break;
    case ISA::OP_FETCHR: instruction = new A<FetchR>(encoded); break;
    case ISA::OP_FETCH: instruction = new A<Fetch>(encoded); break;
    case ISA::OP_STB: instruction = new A<StB>(encoded); break;
    case ISA::OP_FILLR: instruction = new A<FillR>(encoded); break;
    case ISA::OP_FETCHPST: instruction = new A<FetchPst>(encoded); break;
    case ISA::OP_LUI: instruction = new A<LUI>(encoded); break;
    case ISA::OP_FETCHPSTR: instruction = new A<FetchPstR>(encoded); break;
    case ISA::OP_FILL: instruction = new A<Fill>(encoded); break;
    case ISA::OP_CREGWRI: instruction = new A<CRegWrI>(encoded); break;
    case ISA::OP_PSEL_FETCHR: instruction = new A<PSelFetchR>(encoded); break;
    case ISA::OP_SELCH: instruction = new A<SelCh>(encoded); break;
    case ISA::OP_IRDR: instruction = new A<IRdR>(encoded); break;
    case ISA::OP_CREGRDI: instruction = new A<CRegRdI>(encoded); break;
    case ISA::OP_TSTCHI: instruction = new A<TstChI>(encoded); break;
    case ISA::OP_TSTCHI_P: instruction = new A<WritePredicate<TstChI>>(encoded); break;
    case ISA::OP_GETCHMAPI: instruction = new A<GetChMapI>(encoded); break;
    case ISA::OP_GETCHMAP: instruction = new A<GetChMap>(encoded); break;
    case ISA::OP_LLI: instruction = new A<LLI>(encoded); break;
    case ISA::OP_SCRATCHRDI: instruction = new A<ScratchRdI>(encoded); break;
    case ISA::OP_SCRATCHRD: instruction = new A<ScratchRd>(encoded); break;

    default:
      assert(false); instruction = NULL; break;
  }

  return DecodedInstruction(instruction);
}
