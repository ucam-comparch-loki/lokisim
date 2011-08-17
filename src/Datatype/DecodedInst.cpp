/*
 * DecodedInst.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#include "DecodedInst.h"
#include "Instruction.h"
#include "MemoryRequest.h"

const operation_t   DecodedInst::operation()       const {return operation_;}
const RegisterIndex DecodedInst::sourceReg1()      const {return sourceReg1_;}
const RegisterIndex DecodedInst::sourceReg2()      const {return sourceReg2_;}
const RegisterIndex DecodedInst::destination()     const {return destReg_;}
const int32_t       DecodedInst::immediate()       const {return immediate_;}
const ChannelIndex  DecodedInst::channelMapEntry() const {return channelMapEntry_;}
const uint8_t       DecodedInst::predicate()       const {return predicate_;}
const bool          DecodedInst::setsPredicate()   const {return setsPred_;}
const uint8_t       DecodedInst::memoryOp()        const {return memoryOp_;}

const int32_t       DecodedInst::operand1()        const {return operand1_;}
const int32_t       DecodedInst::operand2()        const {return operand2_;}
const int64_t       DecodedInst::result()          const {return result_;}

const MemoryAddr    DecodedInst::location()        const {return location_;}


const bool    DecodedInst::usesPredicate() const {
  return (predicate_ == Instruction::NOT_P) || (predicate_ == Instruction::P) ||
         (operation_ == InstructionMap::PSEL) || (operation_ == InstructionMap::PSELFETCH);
}

const bool    DecodedInst::hasResult()             const {return hasResult_;}


const bool    DecodedInst::hasDestReg() const {
  return InstructionMap::hasDestReg(operation_);
}

const bool    DecodedInst::hasSrcReg1() const {
  return InstructionMap::hasSrcReg1(operation_);
}

const bool    DecodedInst::hasSrcReg2() const {
  return InstructionMap::hasSrcReg2(operation_);
}

const bool    DecodedInst::hasImmediate() const {
  return InstructionMap::hasImmediate(operation_);
}

const bool    DecodedInst::isALUOperation() const {
  return InstructionMap::isALUOperation(operation_);
}

const bool    DecodedInst::endOfPacket() const {
  return predicate() == Instruction::END_OF_PACKET;
}

const std::string& DecodedInst::name() const {
  return InstructionMap::name(operation_);
}


void DecodedInst::operation(const operation_t val) 				{operation_ = val;}
void DecodedInst::sourceReg1(const RegisterIndex val)     {sourceReg1_ = val;}
void DecodedInst::sourceReg2(const RegisterIndex val)     {sourceReg2_ = val;}
void DecodedInst::destination(const RegisterIndex val)    {destReg_ = val;}
void DecodedInst::immediate(const int32_t val)            {immediate_ = val;}
void DecodedInst::channelMapEntry(const ChannelIndex val) {channelMapEntry_ = val;}
void DecodedInst::predicate(const uint8_t val)            {predicate_ = val;}
void DecodedInst::setsPredicate(const bool val)           {setsPred_ = val;}
void DecodedInst::memoryOp(const uint8_t val)             {memoryOp_ = val;}

void DecodedInst::operand1(const int32_t val)             {operand1_ = val;}
void DecodedInst::operand2(const int32_t val)             {operand2_ = val;}

void DecodedInst::result(const int64_t val) {
  result_ = val;
  hasResult_ = true;
}

void DecodedInst::location(const MemoryAddr val)          {location_ = val;}


Instruction DecodedInst::toInstruction() const {
  Instruction i;
  i.opcode(InstructionMap::opcode(name()));
  i.immediate(immediate_);
  i.remoteChannel(channelMapEntry_);
  i.predicate(predicate_);
  i.setsPredicate(setsPred_);

  switch(operation_) {
    // Single source, no destination.
    case InstructionMap::LDW :
    case InstructionMap::LDHWU :
    case InstructionMap::LDBU :
    case InstructionMap::STWADDR :
    case InstructionMap::STBADDR :
    case InstructionMap::FETCH :
    case InstructionMap::FETCHPST :
    case InstructionMap::RMTFETCH :
    case InstructionMap::RMTFETCHPST :
    case InstructionMap::RMTFILL :
    case InstructionMap::SETCHMAP : {
      i.destination(sourceReg1_);
      i.sourceReg1(0);
      i.sourceReg2(0);
      break;
    }

    // Two sources, no destination.
    case InstructionMap::STW :
    case InstructionMap::STHW :
    case InstructionMap::STB :
    case InstructionMap::PSELFETCH : {
      i.destination(sourceReg1_);
      i.sourceReg1(sourceReg2_);
      i.sourceReg2(0);
      break;
    }

    // Two sources and a destination.
    default : {
      i.destination(destReg_);
      i.sourceReg1(sourceReg1_);
      i.sourceReg2(sourceReg2_);
      break;
    }
  }

  return i;
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

  os << name() << (setsPredicate() ? ".p" : "")
     << (predicate()==Instruction::END_OF_PACKET ? ".eop" : "");

  // Special case for cfgmem and setchmap: immediate is printed before register.
  if(operation_ == InstructionMap::SETCHMAP) {
    os << " " << immediate() << ", r" << (int)sourceReg1();
    return os;
  }

  // Special case for loads: print the form 8(r2).
  if(operation_ >= InstructionMap::LDW && operation_ <= InstructionMap::LDBU) {
    os << " " << immediate() << "(r" << (int)sourceReg1() << ")";
  }
  // Special case for stores: print the form r3 8(r2).
  else if(operation_ >= InstructionMap::STW && operation_ <= InstructionMap::STB) {
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
}

DecodedInst::DecodedInst(const Instruction i) {
  operation_       = InstructionMap::operation(i.opcode());
  immediate_       = i.immediate();
  channelMapEntry_ = i.remoteChannel();
  predicate_       = i.predicate();
  setsPred_        = i.setsPredicate();
  memoryOp_        = MemoryRequest::NONE;

  operand1_        = 0;
  operand2_        = 0;
  result_          = 0;

  hasResult_       = false;

  switch(operation_) {
    // Single source, no destination.
    case InstructionMap::LDW :
    case InstructionMap::LDHWU :
    case InstructionMap::LDBU :
    case InstructionMap::STWADDR :
    case InstructionMap::STBADDR :
    case InstructionMap::FETCH :
    case InstructionMap::FETCHPST :
    case InstructionMap::RMTFETCH :
    case InstructionMap::RMTFETCHPST :
    case InstructionMap::RMTFILL :
    case InstructionMap::SETCHMAP : {
      destReg_ = 0;
      sourceReg1_ = i.destination();
      sourceReg2_ = 0;
      break;
    }

    // Two sources, no destination.
    case InstructionMap::STW :
    case InstructionMap::STHW :
    case InstructionMap::STB :
    case InstructionMap::PSELFETCH : {
      destReg_ = 0;
      sourceReg1_ = i.destination();
      sourceReg2_ = i.sourceReg1();
      break;
    }

    // Two sources and a destination.
    default : {
      destReg_ = i.destination();
      sourceReg1_ = i.sourceReg1();
      sourceReg2_ = i.sourceReg2();
      break;
    }
  }
}
