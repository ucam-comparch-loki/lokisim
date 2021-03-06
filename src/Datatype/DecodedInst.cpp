/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"

#include "../Utility/Logging.h"
opcode_t      DecodedInst::opcode()          const {return opcode_;}
function_t    DecodedInst::function()        const {return function_;}
format_t      DecodedInst::format()          const {return format_;}
RegisterIndex DecodedInst::sourceReg1()      const {return sourceReg1_;}
RegisterIndex DecodedInst::sourceReg2()      const {return sourceReg2_;}
RegisterIndex DecodedInst::destination()     const {return destReg_;}

int32_t       DecodedInst::immediate()       const {
  if (opcode_ == ISA::OP_PSEL_FETCHR)
    return immediate_ >> 7;
  else
    return immediate_;
}
int32_t       DecodedInst::immediate2()      const {
  if (opcode_ == ISA::OP_PSEL_FETCHR)
    return (immediate_ << 25) >> 25;
  else
    return 0;
}

ChannelIndex  DecodedInst::channelMapEntry() const {return channelMapEntry_;}
predicate_t   DecodedInst::predicate()       const {return predicate_;}
bool          DecodedInst::setsPredicate()   const {return setsPred_;}

MemoryOpcode DecodedInst::memoryOp() const {
  if (opcode() == ISA::OP_SENDCONFIG)
    return MemoryMetadata(immediate()).opcode;
  else
    return memoryOp_;
}

typedef DecodedInst::OperandSource OperandSource;
OperandSource DecodedInst::operand1Source()  const {return op1Source_;}
OperandSource DecodedInst::operand2Source()  const {return op2Source_;}
bool          DecodedInst::hasOperand1()     const {return op1Source_ != NONE;}
bool          DecodedInst::hasOperand2()     const {return op2Source_ != NONE;}

int32_t       DecodedInst::operand1()        const {return operand1_;}
int32_t       DecodedInst::operand2()        const {return operand2_;}
int64_t       DecodedInst::result()          const {return result_;}

EncodedCMTEntry DecodedInst::cmtEntry()      const {return networkInfo;}

const ChannelID DecodedInst::networkDestination() const {
  // FIXME: this code is very similar to some in ChannelMapEntry. It would be
  // nice to merge them.

  if (cmtEntry() == 0) {
    return ChannelID();
  }
  else if (ChannelMapEntry::globalView(cmtEntry()).isGlobal) {
    ChannelMapEntry::GlobalChannel channel(cmtEntry());
    return ChannelID(channel.tileX, channel.tileY, channel.core, channel.channel);
  }
  else if (ChannelMapEntry::memoryView(cmtEntry()).isMemory) {
    ChannelMapEntry::MemoryChannel channel(cmtEntry());
    return ChannelID(0, 0, channel.bank+CORES_PER_TILE, channel.channel);
  }
  else {
    ChannelMapEntry::MulticastChannel channel(cmtEntry());
    return ChannelID(channel.coreMask, channel.channel);
  }
}

MemoryAddr    DecodedInst::location()        const {return location_;}
InstructionSource DecodedInst::source()      const {return source_;}


bool    DecodedInst::predicated() const {
  return (predicate_ == Instruction::NOT_P) || (predicate_ == Instruction::P);
}

bool    DecodedInst::hasResult()             const {return hasResult_;}


bool    DecodedInst::hasDestReg() const {
  return ISA::hasDestReg(opcode_);
}

bool    DecodedInst::hasSrcReg1() const {
  return ISA::hasSrcReg1(opcode_);
}

bool    DecodedInst::hasSrcReg2() const {
  return ISA::hasSrcReg2(opcode_);
}

bool    DecodedInst::hasImmediate() const {
  return ISA::hasImmediate(opcode_);
}

bool    DecodedInst::isDecodeStageOperation() const {
  switch (opcode_) {
    case ISA::OP_RMTNXIPK:
    case ISA::OP_RMTEXECUTE:
    case ISA::OP_PSEL_FETCH:
    case ISA::OP_PSEL_FETCHR:
    case ISA::OP_FETCH:
    case ISA::OP_FETCHR:
    case ISA::OP_FETCHPST:
    case ISA::OP_FETCHPSTR:
    case ISA::OP_FILL:
    case ISA::OP_FILLR:
    case ISA::OP_IBJMP:
    case ISA::OP_IRDR:
    case ISA::OP_WOCHE:
    case ISA::OP_SELCH:
    case ISA::OP_TSTCHI:
    case ISA::OP_TSTCHI_P:
      return true;
    default:
      return false;
  }
}

bool    DecodedInst::isExecuteStageOperation() const {
  switch (opcode_) {
    case ISA::OP_SCRATCHRD:
    case ISA::OP_SCRATCHRDI:
    case ISA::OP_SCRATCHWR:
    case ISA::OP_SCRATCHWRI:
    case ISA::OP_CREGRDI:
    case ISA::OP_CREGWRI:
    case ISA::OP_GETCHMAP:
    case ISA::OP_GETCHMAPI:
    case ISA::OP_SETCHMAP:
    case ISA::OP_SETCHMAPI:
    case ISA::OP_SYSCALL:
      return true;
    default:
      return ISA::isALUOperation(opcode_);
  }
}

bool    DecodedInst::endOfIPK() const {
  return predicate() == Instruction::END_OF_PACKET;
}

bool    DecodedInst::persistent() const {
  return persistent_;
}

bool    DecodedInst::endOfNetworkPacket() const {
  if (opcode() == ISA::OP_SENDCONFIG)
    return FlitMetadata(immediate()).endOfPacket;
  else
    return endOfPacket_;
}

bool    DecodedInst::forRemoteExecution() const {
  return forRemoteExecution_;
}

const inst_name_t& DecodedInst::name() const {
  return ISA::name(opcode_, function_);
}


void DecodedInst::opcode(const opcode_t val)              {opcode_ = val;}
void DecodedInst::function(const function_t val)          {function_ = val;}
void DecodedInst::sourceReg1(const RegisterIndex val)     {sourceReg1_ = val;}
void DecodedInst::sourceReg2(const RegisterIndex val)     {sourceReg2_ = val;}
void DecodedInst::destination(const RegisterIndex val)    {destReg_ = val;}
void DecodedInst::immediate(const int32_t val)            {immediate_ = val;}
void DecodedInst::channelMapEntry(const ChannelIndex val) {channelMapEntry_ = val;}
void DecodedInst::predicate(const predicate_t val)        {predicate_ = val;}
void DecodedInst::setsPredicate(const bool val)           {setsPred_ = val;}
void DecodedInst::memoryOp(const MemoryOpcode val)        {memoryOp_ = val;}

void DecodedInst::operand1Source(const OperandSource src) {op1Source_ = src;}
void DecodedInst::operand2Source(const OperandSource src) {op2Source_ = src;}

void DecodedInst::operand1(const int32_t val)             {operand1_ = val;}
void DecodedInst::operand2(const int32_t val)             {operand2_ = val;}

void DecodedInst::result(const int64_t val) {
  result_ = val;
  hasResult_ = true;
}

void DecodedInst::persistent(const bool val)              {persistent_ = val;}
void DecodedInst::remoteExecute(const bool val)           {forRemoteExecution_ = val;}
void DecodedInst::endOfNetworkPacket(const bool val)      {endOfPacket_ = val;}

void DecodedInst::cmtEntry(const EncodedCMTEntry val)     {networkInfo = val;}
void DecodedInst::portClaim(const bool val)               {portClaim_ = val;}

void DecodedInst::location(const MemoryAddr val)          {location_ = val;}
void DecodedInst::source(const InstructionSource val)     {source_ = val;}


Instruction DecodedInst::toInstruction() const {
  return original_;
}

bool DecodedInst::sendsOnNetwork() const {
  return (channelMapEntry() != Instruction::NO_CHANNEL) &&
         !networkDestination().isNullMapping();
}

bool DecodedInst::storesToRegister() const {
  return destReg_ != 0;
}

const NetworkData DecodedInst::toNetworkData(TileID tile) const {
  ChannelID destination = networkDestination();
  if (ChannelMapEntry::isMemory(networkInfo))
    destination.component.tile = tile;

  NetworkData flit;

  if (opcode() == ISA::OP_SENDCONFIG)
    flit = NetworkData(result(), destination, uint32_t(immediate()));
  else if (destination.multicast) {
    // We never acquire channels on the local networks, so just provide "false".
    flit = NetworkData(result(), destination, false, portClaim_, endOfNetworkPacket());
  }
  else if (ChannelMapEntry::isCore(networkInfo)) {
    ChannelMapEntry::GlobalChannel channel(networkInfo);
    flit = NetworkData(result(), destination, channel.acquired, portClaim_, endOfNetworkPacket());
  }
  else {
    assert(ChannelMapEntry::isMemory(networkInfo));
    flit = NetworkData(result(),
                       destination,
                       ChannelMapEntry::memoryView(networkInfo),
                       memoryOp(),
                       endOfNetworkPacket());
  }

  flit.setInstruction(forRemoteExecution_);
  return flit;
}

void DecodedInst::preventForwarding() {
  // No instruction reads data from register 0, so the result will not be
  // forwarded.
  destReg_ = 0;

  // Also need to show that the predicate will not change, so no instructions
  // wait for it.
  setsPred_ = false;

  // If any instruction was waiting for this one to finish, signal that it
  // has finished.
  hasResult_ = true;
}


bool DecodedInst::operator== (const DecodedInst& other) const {
  // Is it safe to assume that if two instructions come from the same memory
  // address, they are the same instruction?
  // As long as we don't have dynamic code rewriting...
  return location_ == other.location_;
}

DecodedInst& DecodedInst::operator= (const DecodedInst& other) {
  memcpy(this, &other, sizeof(other));
  return *this;
}

std::ostream& DecodedInst::print(std::ostream& os) const {
  if (location_ != 0)
    os << LOKI_HEX(location_) << " ";

  if (predicate() == Instruction::P) os << "ifp?";
  else if (predicate() == Instruction::NOT_P) os << "if!p?";

  os << name() << (predicate()==Instruction::END_OF_PACKET ? ".eop" : "");

  // Special case for cfgmem and setchmap: immediate is printed before register.
  if (opcode_ == ISA::OP_SETCHMAP || opcode_ == ISA::OP_SETCHMAPI) {
    os << " " << immediate() << ", r" << (int)sourceReg1();
    return os;
  }

  // Special case for loads: print the form 8(r2).
  if (opcode_ == ISA::OP_LDW ||
      opcode_ == ISA::OP_LDHWU ||
      opcode_ == ISA::OP_LDBU) {
    os << " " << immediate() << "(r" << (int)sourceReg1() << ")";
  }
  // Special case for stores: print the form r3 8(r2).
  else if (opcode_ == ISA::OP_STW ||
           opcode_ == ISA::OP_STHW ||
           opcode_ == ISA::OP_STB) {
    os << " r" << (int)sourceReg1() << ", " << immediate() << "(r"
       << (int)sourceReg2() << ")";
  }
  // Default case.
  else {
    // Figure out where commas are needed.
    bool fieldAfterSrc2 = hasImmediate();
    bool fieldAfterSrc1 = fieldAfterSrc2 || hasSrcReg2();
    bool fieldAfterDest = fieldAfterSrc1 || hasSrcReg1();

    if (hasDestReg()) os << " r" << (int)destination() << (fieldAfterDest?",":"");
    if (hasSrcReg1()) os << " r" << (int)sourceReg1() << (fieldAfterSrc1?",":"");
    if (hasSrcReg2()) os << " r" << (int)sourceReg2() << (fieldAfterSrc2?",":"");
    if (hasImmediate()) os << " "  << immediate();
  }

  if (channelMapEntry() != Instruction::NO_CHANNEL)
    os << " -> " << (int)channelMapEntry();

  return os;
}

void DecodedInst::init() {
  opcode_           = static_cast<opcode_t>(0);
  function_         = static_cast<function_t>(0);
  format_           = static_cast<format_t>(0);
  destReg_          = 0;
  sourceReg1_       = 0;
  sourceReg2_       = 0;
  immediate_        = 0;
  channelMapEntry_  = 0;
  predicate_        = static_cast<predicate_t>(0);
  setsPred_         = false;
  memoryOp_         = MemoryOpcode::LOAD_W;  // non-zero
  op1Source_        = OperandSource::NONE;
  op2Source_        = OperandSource::NONE;
  operand1_         = 0;
  operand2_         = 0;
  result_           = 0;
  portClaim_        = false;
  persistent_       = false;
  forRemoteExecution_ = false;
  endOfPacket_      = true;                 // non-zero
  networkInfo       = 0;
  location_         = 0;
  source_           = UNKNOWN;
  hasResult_        = false;
}

DecodedInst::DecodedInst() {
  init();
}

DecodedInst::DecodedInst(const Instruction inst) : original_(inst) {
  init();

  predicate_       = inst.predicate();
  opcode_          = inst.opcode();

  // The first two opcodes have the ALU function encoded as well. For all
  // others, we must look it up.
  if (opcode_ == 0 || opcode_ == 1)
    function_      = inst.function();
  else
    function_      = ISA::function(opcode_);

  // Determine which channel (if any) this instruction sends its result to.
  // Most instructions encode this explicitly, but fetches implicitly send to
  // channel 0.
  switch (opcode_) {
    case ISA::OP_FETCHR:
    case ISA::OP_FETCHPSTR:
    case ISA::OP_FILLR:
    case ISA::OP_PSEL_FETCHR:
    case ISA::OP_FETCH:
    case ISA::OP_FETCHPST:
    case ISA::OP_FILL:
    case ISA::OP_PSEL_FETCH:
      channelMapEntry_ = 0;
      break;
    default:
      if (ISA::hasRemoteChannel(opcode_))
        channelMapEntry_ = inst.remoteChannel();
      else
        channelMapEntry_ = Instruction::NO_CHANNEL;
      break;
  }

  setsPred_        = ISA::setsPredicate(opcode_);
  format_          = ISA::format(opcode_);

  bool signedImmed = ISA::hasSignedImmediate(opcode_);

  // Different instruction formats have immediate fields of different sizes.
  switch (format_) {
    case ISA::FMT_FF:
    case ISA::FMT_PFF:
      if (signedImmed) immediate_ = signextend<int32_t,  23>(inst.immediate());
      else             immediate_ = signextend<uint32_t, 23>(inst.immediate());
      break;

    case ISA::FMT_0R:
    case ISA::FMT_0Rnc:
    case ISA::FMT_1R:
      if (signedImmed) immediate_ = signextend<int32_t,  14>(inst.immediate());
      else             immediate_ = signextend<uint32_t, 14>(inst.immediate());
      break;

    case ISA::FMT_1Rnc:
      if (signedImmed) immediate_ = signextend<int32_t,  16>(inst.immediate());
      else             immediate_ = signextend<uint32_t, 16>(inst.immediate());
      break;

    case ISA::FMT_2R:
    case ISA::FMT_2Rnc:
      if (signedImmed) immediate_ = signextend<int32_t,   9>(inst.immediate());
      else             immediate_ = signextend<uint32_t,  9>(inst.immediate());
      break;

    case ISA::FMT_2Rs:
      immediate_ = signextend<uint32_t, 5>(inst.immediate());
      break;

    case ISA::FMT_3R:
      immediate_ =  0;
      break;
  }

  // "Unpack" the registers held in the encoded instruction. For example, the
  // first register could be a destination or a source, or may not be defined
  // at all.
  int registersRetrieved = 0;
  for (int registersAssigned=0; registersAssigned<3; registersAssigned++) {
    RegisterIndex reg;

    if (registersRetrieved == 0)      reg = inst.reg1();
    else if (registersRetrieved == 1) reg = inst.reg2();
    else                              reg = inst.reg3();

    if (registersAssigned == 0) {
      if (ISA::hasDestReg(opcode_)) {
        destReg_ = reg;
        registersRetrieved++;
      }
      else destReg_ = 0;
    }
    else if (registersAssigned == 1) {
      if (ISA::hasSrcReg1(opcode_)) {
        sourceReg1_ = reg;
        registersRetrieved++;
      }
      else sourceReg1_ = 0;
    }
    else {
      if (ISA::hasSrcReg2(opcode_)) {
        sourceReg2_ = reg;
        registersRetrieved++;
      }
      else sourceReg2_ = 0;
    }
  }
}
