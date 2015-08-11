/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"

const opcode_t      DecodedInst::opcode()          const {return opcode_;}
const function_t    DecodedInst::function()        const {return function_;}
const format_t      DecodedInst::format()          const {return format_;}
const RegisterIndex DecodedInst::sourceReg1()      const {return sourceReg1_;}
const RegisterIndex DecodedInst::sourceReg2()      const {return sourceReg2_;}
const RegisterIndex DecodedInst::destination()     const {return destReg_;}

const int32_t       DecodedInst::immediate()       const {
  if (opcode_ == InstructionMap::OP_PSEL_FETCHR)
    return immediate_ >> 7;
  else
    return immediate_;
}
const int32_t       DecodedInst::immediate2()      const {
  if (opcode_ == InstructionMap::OP_PSEL_FETCHR)
    return (immediate_ << 25) >> 25;
  else
    return 0;
}

const ChannelIndex  DecodedInst::channelMapEntry() const {return channelMapEntry_;}
const predicate_t   DecodedInst::predicate()       const {return predicate_;}
const bool          DecodedInst::setsPredicate()   const {return setsPred_;}

const MemoryOperation DecodedInst::memoryOp() const {
  if (opcode() == InstructionMap::OP_SENDCONFIG)
    return MemoryMetadata(immediate()).opcode;
  else
    return memoryOp_;
}

typedef DecodedInst::OperandSource OperandSource;
const OperandSource DecodedInst::operand1Source()  const {return op1Source_;}
const OperandSource DecodedInst::operand2Source()  const {return op2Source_;}
const bool          DecodedInst::hasOperand1()     const {return op1Source_ != NONE;}
const bool          DecodedInst::hasOperand2()     const {return op2Source_ != NONE;}

const int32_t       DecodedInst::operand1()        const {return operand1_;}
const int32_t       DecodedInst::operand2()        const {return operand2_;}
const int64_t       DecodedInst::result()          const {return result_;}

const EncodedCMTEntry DecodedInst::cmtEntry()      const {return networkInfo;}

const ChannelID     DecodedInst::networkDestination() const {
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

const MemoryAddr    DecodedInst::location()        const {return location_;}


const bool    DecodedInst::predicated() const {
  return (predicate_ == Instruction::NOT_P) || (predicate_ == Instruction::P);
}

const bool    DecodedInst::hasResult()             const {return hasResult_;}


const bool    DecodedInst::hasDestReg() const {
  return InstructionMap::hasDestReg(opcode_);
}

const bool    DecodedInst::hasSrcReg1() const {
  return InstructionMap::hasSrcReg1(opcode_);
}

const bool    DecodedInst::hasSrcReg2() const {
  return InstructionMap::hasSrcReg2(opcode_);
}

const bool    DecodedInst::hasImmediate() const {
  return InstructionMap::hasImmediate(opcode_);
}

const bool    DecodedInst::isDecodeStageOperation() const {
  switch (opcode_) {
    case InstructionMap::OP_RMTNXIPK:
    case InstructionMap::OP_RMTEXECUTE:
    case InstructionMap::OP_PSEL_FETCH:
    case InstructionMap::OP_PSEL_FETCHR:
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_FILLR:
    case InstructionMap::OP_IBJMP:
    case InstructionMap::OP_IRDR:
    case InstructionMap::OP_WOCHE:
    case InstructionMap::OP_SELCH:
    case InstructionMap::OP_TSTCHI:
    case InstructionMap::OP_TSTCHI_P:
      return true;
    default:
      return false;
  }
}

const bool    DecodedInst::isExecuteStageOperation() const {
  switch (opcode_) {
    case InstructionMap::OP_SCRATCHRD:
    case InstructionMap::OP_SCRATCHRDI:
    case InstructionMap::OP_SCRATCHWR:
    case InstructionMap::OP_SCRATCHWRI:
    case InstructionMap::OP_CREGRDI:
    case InstructionMap::OP_CREGWRI:
    case InstructionMap::OP_GETCHMAP:
    case InstructionMap::OP_GETCHMAPI:
    case InstructionMap::OP_SETCHMAP:
    case InstructionMap::OP_SETCHMAPI:
    case InstructionMap::OP_SYSCALL:
      return true;
    default:
      return InstructionMap::isALUOperation(opcode_);
  }
}

const bool    DecodedInst::endOfIPK() const {
  return predicate() == Instruction::END_OF_PACKET;
}

const bool    DecodedInst::persistent() const {
  return persistent_;
}

const bool    DecodedInst::endOfNetworkPacket() const {
  if (opcode() == InstructionMap::OP_SENDCONFIG)
    return FlitMetadata(immediate()).endOfPacket;
  else
    return endOfPacket_;
}

const inst_name_t& DecodedInst::name() const {
  return InstructionMap::name(opcode_, function_);
}


void DecodedInst::opcode(const opcode_t val) 				      {opcode_ = val;}
void DecodedInst::function(const function_t val)          {function_ = val;}
void DecodedInst::sourceReg1(const RegisterIndex val)     {sourceReg1_ = val;}
void DecodedInst::sourceReg2(const RegisterIndex val)     {sourceReg2_ = val;}
void DecodedInst::destination(const RegisterIndex val)    {destReg_ = val;}
void DecodedInst::immediate(const int32_t val)            {immediate_ = val;}
void DecodedInst::channelMapEntry(const ChannelIndex val) {channelMapEntry_ = val;}
void DecodedInst::predicate(const predicate_t val)        {predicate_ = val;}
void DecodedInst::setsPredicate(const bool val)           {setsPred_ = val;}
void DecodedInst::memoryOp(const MemoryOperation val)     {memoryOp_ = val;}

void DecodedInst::operand1Source(const OperandSource src) {op1Source_ = src;}
void DecodedInst::operand2Source(const OperandSource src) {op2Source_ = src;}

void DecodedInst::operand1(const int32_t val)             {operand1_ = val;}
void DecodedInst::operand2(const int32_t val)             {operand2_ = val;}

void DecodedInst::result(const int64_t val) {
  result_ = val;
  hasResult_ = true;
}

void DecodedInst::persistent(const bool val)              {persistent_ = val;}
void DecodedInst::endOfNetworkPacket(const bool val)      {endOfPacket_ = val;}

void DecodedInst::cmtEntry(const EncodedCMTEntry val)     {networkInfo = val;}
void DecodedInst::portClaim(const bool val)               {portClaim_ = val;}

void DecodedInst::location(const MemoryAddr val)          {location_ = val;}


Instruction DecodedInst::toInstruction() const {
  return original_;
}

const bool DecodedInst::sendsOnNetwork() const {
  return (channelMapEntry() != Instruction::NO_CHANNEL) &&
         !networkDestination().isNullMapping();
}

const bool DecodedInst::storesToRegister() const {
  return destReg_ != 0;
}

const NetworkData DecodedInst::toNetworkData(TileID tile) const {
  ChannelID destination = networkDestination();
  if (destination.isMemory())
    destination.component.tile = tile;

  if (opcode() == InstructionMap::OP_SENDCONFIG)
    return NetworkData(result(), destination, uint32_t(immediate()));
  else if (destination.multicast) {
    // We never acquire channels on the local networks, so just provide "false".
    return NetworkData(result(), destination, false, portClaim_, endOfNetworkPacket());
  }
  else if (destination.isCore()) {
    ChannelMapEntry::GlobalChannel channel(networkInfo);
    return NetworkData(result(), destination, channel.acquired, portClaim_, endOfNetworkPacket());
  }
  else {
    assert(destination.isMemory());
    return NetworkData(result(),
                       destination,
                       ChannelMapEntry::memoryView(networkInfo),
                       memoryOp(),
                       endOfNetworkPacket());
  }
}

void DecodedInst::isid(const unsigned long long isid) const {
  isid_ = isid;
}

const unsigned long long DecodedInst::isid() const {
  return isid_;
}

void DecodedInst::preventForwarding() {
  // No instruction reads data from register 0, so the result will not be
  // forwarded.
  destReg_ = 0;

  // Also need to show that the predicate will not change, so no instructions
  // wait for it.
  setsPred_ = false;
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
  if (predicate() == Instruction::P) os << "ifp?";
  else if (predicate() == Instruction::NOT_P) os << "if!p?";

  os << name() << (predicate()==Instruction::END_OF_PACKET ? ".eop" : "");

  // Special case for cfgmem and setchmap: immediate is printed before register.
  if (opcode_ == InstructionMap::OP_SETCHMAP || opcode_ == InstructionMap::OP_SETCHMAPI) {
    os << " " << immediate() << ", r" << (int)sourceReg1();
    return os;
  }

  // Special case for loads: print the form 8(r2).
  if (opcode_ == InstructionMap::OP_LDW ||
      opcode_ == InstructionMap::OP_LDHWU ||
      opcode_ == InstructionMap::OP_LDBU) {
    os << " " << immediate() << "(r" << (int)sourceReg1() << ")";
  }
  // Special case for stores: print the form r3 8(r2).
  else if (opcode_ == InstructionMap::OP_STW ||
           opcode_ == InstructionMap::OP_STHW ||
           opcode_ == InstructionMap::OP_STB) {
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
  memoryOp_         = MemoryOperation::LOAD_W;  // non-zero
  op1Source_        = OperandSource::NONE;
  op2Source_        = OperandSource::NONE;
  operand1_         = 0;
  operand2_         = 0;
  result_           = 0;
  portClaim_        = false;
  persistent_       = false;
  endOfPacket_      = true;                 // non-zero
  networkInfo       = 0;
  location_         = 0;
  hasResult_        = false;
  isid_             = -1;                   // Not sure about this
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
    function_      = InstructionMap::function(opcode_);

  // Determine which channel (if any) this instruction sends its result to.
  // Most instructions encode this explicitly, but fetches implicitly send to
  // channel 0.
  switch (opcode_) {
    case InstructionMap::OP_FETCHR:
    case InstructionMap::OP_FETCHPSTR:
    case InstructionMap::OP_FILLR:
    case InstructionMap::OP_PSEL_FETCHR:
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_PSEL_FETCH:
      channelMapEntry_ = 0;
      break;
    default:
      if (InstructionMap::hasRemoteChannel(opcode_))
        channelMapEntry_ = inst.remoteChannel();
      else
        channelMapEntry_ = Instruction::NO_CHANNEL;
      break;
  }

  setsPred_        = InstructionMap::setsPredicate(opcode_);
  format_          = InstructionMap::format(opcode_);

  bool signedImmed = InstructionMap::hasSignedImmediate(opcode_);

  // Different instruction formats have immediate fields of different sizes.
  switch (format_) {
    case InstructionMap::FMT_FF:
    case InstructionMap::FMT_PFF:
      if (signedImmed) immediate_ = signextend<int32_t,  23>(inst.immediate());
      else             immediate_ = signextend<uint32_t, 23>(inst.immediate());
      break;

    case InstructionMap::FMT_0R:
    case InstructionMap::FMT_0Rnc:
    case InstructionMap::FMT_1R:
      if (signedImmed) immediate_ = signextend<int32_t,  14>(inst.immediate());
      else             immediate_ = signextend<uint32_t, 14>(inst.immediate());
      break;

    case InstructionMap::FMT_1Rnc:
      if (signedImmed) immediate_ = signextend<int32_t,  16>(inst.immediate());
      else             immediate_ = signextend<uint32_t, 16>(inst.immediate());
      break;

    case InstructionMap::FMT_2R:
    case InstructionMap::FMT_2Rnc:
      if (signedImmed) immediate_ = signextend<int32_t,   9>(inst.immediate());
      else             immediate_ = signextend<uint32_t,  9>(inst.immediate());
      break;

    case InstructionMap::FMT_2Rs:
      immediate_ = signextend<uint32_t, 5>(inst.immediate());
      break;

    case InstructionMap::FMT_3R:
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
      if (InstructionMap::hasDestReg(opcode_)) {
        destReg_ = reg;
        registersRetrieved++;
      }
      else destReg_ = 0;
    }
    else if (registersAssigned == 1) {
      if (InstructionMap::hasSrcReg1(opcode_)) {
        sourceReg1_ = reg;
        registersRetrieved++;
      }
      else sourceReg1_ = 0;
    }
    else {
      if (InstructionMap::hasSrcReg2(opcode_)) {
        sourceReg2_ = reg;
        registersRetrieved++;
      }
      else sourceReg2_ = 0;
    }
  }
}
