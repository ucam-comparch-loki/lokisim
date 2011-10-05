/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"
#include "MemoryRequest.h"

const opcode_t      DecodedInst::opcode()       const {return opcode_;}
const function_t    DecodedInst::function()        const {return function_;}
const format_t      DecodedInst::format()          const {return format_;}
const RegisterIndex DecodedInst::sourceReg1()      const {return sourceReg1_;}
const RegisterIndex DecodedInst::sourceReg2()      const {return sourceReg2_;}
const RegisterIndex DecodedInst::destination()     const {return destReg_;}
const int32_t       DecodedInst::immediate()       const {return immediate_;}
const ChannelIndex  DecodedInst::channelMapEntry() const {return channelMapEntry_;}
const predicate_t   DecodedInst::predicate()       const {return predicate_;}
const bool          DecodedInst::setsPredicate()   const {return setsPred_;}
const uint8_t       DecodedInst::memoryOp()        const {return memoryOp_;}

const int32_t       DecodedInst::operand1()        const {return operand1_;}
const int32_t       DecodedInst::operand2()        const {return operand2_;}
const int64_t       DecodedInst::result()          const {return result_;}

const ChannelID     DecodedInst::networkDestination() const {return networkDest_;}
const MemoryAddr    DecodedInst::location()        const {return location_;}


const bool    DecodedInst::usesPredicate() const {
  return (predicate_ == Instruction::NOT_P) || (predicate_ == Instruction::P) ||
         (opcode_ == InstructionMap::OP_PSEL) || (opcode_ == InstructionMap::OP_PSEL_FETCH);
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

const bool    DecodedInst::isALUOperation() const {
  return InstructionMap::isALUOperation(opcode_);
}

const bool    DecodedInst::isMemoryOperation() const {
  return memoryOp_ != MemoryRequest::NONE;
}

const bool    DecodedInst::endOfIPK() const {
  return predicate() == Instruction::END_OF_PACKET;
}

const bool    DecodedInst::endOfNetworkPacket() const {
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
void DecodedInst::memoryOp(const uint8_t val)             {memoryOp_ = val;}

void DecodedInst::operand1(const int32_t val)             {operand1_ = val;}
void DecodedInst::operand2(const int32_t val)             {operand2_ = val;}

void DecodedInst::result(const int64_t val) {
  result_ = val;
  hasResult_ = true;
}

void DecodedInst::endOfNetworkPacket(const bool val)      {endOfPacket_ = val;}
void DecodedInst::portClaim(const bool val)               {portClaim_ = val;}
void DecodedInst::usesCredits(const bool val)             {useCredits_ = val;}
void DecodedInst::networkDestination(const ChannelID val) {networkDest_ = val;}
void DecodedInst::location(const MemoryAddr val)          {location_ = val;}


Instruction DecodedInst::toInstruction() const {
  Instruction i;

  i.immediate(immediate_);

  i.predicate(predicate_);
  i.opcode(InstructionMap::opcode(name()));
  if(InstructionMap::hasRemoteChannel(opcode_))
    i.remoteChannel(channelMapEntry_);


  switch(opcode_) {
    // Single source, no destination.
    case InstructionMap::OP_LDW :
    case InstructionMap::OP_LDHWU :
    case InstructionMap::OP_LDBU :
    case InstructionMap::OP_FETCH :
    case InstructionMap::OP_FETCHPST :
    case InstructionMap::OP_FILL :
    case InstructionMap::OP_SETCHMAP : {
      i.reg1(sourceReg1_);
      //i.reg2(0);
      //i.reg3(0);
      break;
    }

    // Two sources, no destination.
    case InstructionMap::OP_STW :
    case InstructionMap::OP_STHW :
    case InstructionMap::OP_STB :
    case InstructionMap::OP_PSEL_FETCH : {
      i.reg1(sourceReg1_);
      i.reg2(sourceReg2_);
      //i.reg3(0);
      break;
    }

    // Two sources and a destination.
    default : {
      i.reg1(destReg_);
      i.reg2(sourceReg1_);
      i.reg3(sourceReg2_);
      break;
    }
  }

  return i;
}

const bool DecodedInst::sendsOnNetwork() const {
  return channelMapEntry() != Instruction::NO_CHANNEL &&
      !networkDestination().isNullMapping();
}

const AddressedWord DecodedInst::toAddressedWord() const {
  AddressedWord aw(result(), networkDestination());
  aw.setPortClaim(portClaim_, useCredits_);
  aw.setEndOfPacket(endOfPacket_);
  return aw;
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
  if(predicate() == Instruction::P) os << "ifp?";
  else if(predicate() == Instruction::NOT_P) os << "if!p?";

  os << name() << (predicate()==Instruction::END_OF_PACKET ? ".eop" : "");

  // Special case for cfgmem and setchmap: immediate is printed before register.
  if(opcode_ == InstructionMap::OP_SETCHMAP || opcode_ == InstructionMap::OP_SETCHMAPI) {
    os << " " << immediate() << ", r" << (int)sourceReg1();
    return os;
  }

  // Special case for loads: print the form 8(r2).
  if(opcode_ == InstructionMap::OP_LDW ||
     opcode_ == InstructionMap::OP_LDHWU ||
     opcode_ == InstructionMap::OP_LDBU) {
    os << " " << immediate() << "(r" << (int)sourceReg1() << ")";
  }
  // Special case for stores: print the form r3 8(r2).
  else if(opcode_ == InstructionMap::OP_STW ||
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

    if(hasDestReg()) os << " r" << (int)destination() << (fieldAfterDest?",":"");
    if(hasSrcReg1()) os << " r" << (int)sourceReg1() << (fieldAfterSrc1?",":"");
    if(hasSrcReg2()) os << " r" << (int)sourceReg2() << (fieldAfterSrc2?",":"");
    if(hasImmediate()) os << " "  << immediate();
  }

  if(channelMapEntry() != Instruction::NO_CHANNEL)
    os << " -> " << (int)channelMapEntry();

  return os;
}


DecodedInst::DecodedInst() {
  // Everything should default to 0.
  endOfPacket_ = true;
  portClaim_ = false;
  useCredits_ = false;
}

DecodedInst::DecodedInst(const Instruction i) {
  predicate_       = i.predicate();
  opcode_          = i.opcode();

  // The first two opcodes have the ALU function encoded as well. For all
  // others, we must look it up.
  if(opcode_ == 0 || opcode_ == 1)
    function_      = i.function();
  else
    function_      = InstructionMap::function(opcode_);

  if(InstructionMap::hasRemoteChannel(opcode_))
    channelMapEntry_ = i.remoteChannel();
  else
    channelMapEntry_ = Instruction::NO_CHANNEL;

  setsPred_        = InstructionMap::setsPredicate(opcode_);
  format_          = InstructionMap::format(opcode_);
  memoryOp_        = MemoryRequest::NONE;

  operand1_        = 0;
  operand2_        = 0;
  result_          = 0;

  hasResult_       = false;

  endOfPacket_     = true;
  portClaim_       = false;
  useCredits_      = false;

  bool signedImmed = InstructionMap::hasSignedImmediate(opcode_);

  // Different instruction formats have immediate fields of different sizes.
  switch(format_) {
    case InstructionMap::FMT_FF:
      if(signedImmed) immediate_ = signextend<int32_t,  23>(i.immediate());
      else            immediate_ = signextend<uint32_t, 23>(i.immediate());
      break;

    case InstructionMap::FMT_0R:
    case InstructionMap::FMT_0Rnc:
    case InstructionMap::FMT_1R:
      if(signedImmed) immediate_ = signextend<int32_t,  14>(i.immediate());
      else            immediate_ = signextend<uint32_t, 14>(i.immediate());
      break;

    case InstructionMap::FMT_1Rnc:
      if(signedImmed) immediate_ = signextend<int32_t,  16>(i.immediate());
      else            immediate_ = signextend<uint32_t, 16>(i.immediate());
      break;

    case InstructionMap::FMT_2R:
    case InstructionMap::FMT_2Rnc:
      if(signedImmed) immediate_ = signextend<int32_t,   9>(i.immediate());
      else            immediate_ = signextend<uint32_t,  9>(i.immediate());
      break;

    case InstructionMap::FMT_2Rs:
      immediate_ = signextend<uint32_t, 5>(i.immediate());
      break;

    case InstructionMap::FMT_3R:
      immediate_ =  0;
      break;
  }

  // Should format be used instead here?
  switch(opcode_) {
    // No registers
    case InstructionMap::OP_IBJMP:
    case InstructionMap::OP_NXIPK:
    case InstructionMap::OP_SYSCALL:
      destReg_ = 0;
      sourceReg1_ = 0;
      sourceReg2_ = 0;
      break;

    // Single source, no destination.
    case InstructionMap::OP_LDW:
    case InstructionMap::OP_LDHWU:
    case InstructionMap::OP_LDBU:
    case InstructionMap::OP_FETCH:
    case InstructionMap::OP_FETCHPST:
    case InstructionMap::OP_FILL:
    case InstructionMap::OP_SETCHMAP:
    case InstructionMap::OP_SETCHMAPI:
      destReg_    = 0;
      sourceReg1_ = i.reg1();
      sourceReg2_ = 0;
      break;

    // Two sources, no destination.
    case InstructionMap::OP_STW:
    case InstructionMap::OP_STHW:
    case InstructionMap::OP_STB:
    case InstructionMap::OP_PSEL_FETCH:
      destReg_    = 0;
      sourceReg1_ = i.reg1();
      sourceReg2_ = i.reg2();
      break;

    // Two sources and a destination.
    default:
      destReg_    = i.reg1();
      sourceReg1_ = i.reg2();
      sourceReg2_ = i.reg3();
      break;
  }
}
