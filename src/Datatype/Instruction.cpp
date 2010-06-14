/*
 * Instruction.cpp
 *
 * Current layout: 64 bit value containing:
 *    32 bit immediate
 *    1 bit saying whether the predicate register should be set
 *    2 predicate bits
 *    6 bit opcode
 *    5 bit destination register location
 *    5 bit source 1 register location
 *    5 bit source 2 register location
 *    8 bit remote channel ID
 *
 *    | immed | setpred | pred | opcode | dest | source1 | source2 | channel ID |
 *     63      31        30     28       22     17        12        7          0
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Instruction.h"
#include "../Utility/InstructionMap.h"
#include "../Utility/StringManipulation.h"
#include "../Utility/Parameters.h"
#include "../TileComponents/Pipeline/IndirectRegisterFile.h"

typedef StringManipulation Strings;
typedef IndirectRegisterFile Registers;

const short startChannelID = 0;
const short startSrc2      = startChannelID + 8;  // 8
const short startSrc1      = startSrc2 + 5;       // 13
const short startDest      = startSrc1 + 5;       // 18
const short startOpcode    = startDest + 5;       // 23
const short startPredicate = startOpcode + 6;     // 29
const short startSetPred   = startPredicate + 2;  // 31
const short startImmediate = startSetPred + 1;    // 32
const short end            = startImmediate + 32; // 64

/* Public getter methods */
unsigned short Instruction::getOp() const {
  return getBits(startOpcode, startPredicate-1);
}

unsigned short Instruction::getDest() const {
  return getBits(startDest, startOpcode-1);
}

unsigned short Instruction::getSrc1() const {
  return getBits(startSrc1, startDest-1);
}

unsigned short Instruction::getSrc2() const {
  return getBits(startSrc2, startSrc1-1);
}

unsigned short Instruction::getRchannel() const {
  return getBits(startChannelID, startSrc2-1);
}

signed int Instruction::getImmediate() const {
  return getBits(startImmediate, end-1);
}

unsigned short Instruction::getPredicate() const {
  return getBits(startPredicate, startSetPred-1);
}

bool Instruction::getSetPredicate() const {
  return getBits(startSetPred, startImmediate-1);
}

bool Instruction::endOfPacket() const {
  return getPredicate() == END_OF_PACKET;
}


bool Instruction::operator== (const Instruction& other) const {
  return this->data == other.data;
}

/* Constructors and destructors */
Instruction::Instruction() : Word() {
  // Do nothing
}

Instruction::Instruction(const Word& other) : Word(other) {
  // Do nothing
}

Instruction::Instruction(unsigned int inst) : Word(inst) {
  // Do nothing
}

Instruction::Instruction(const string& inst) {

  vector<string> words;

  // Skip this line if it is a comment
  if((inst[0] == '%') || (inst[0] == ';')) throw std::exception();

  // Remove any comments by splitting around ';' and only keeping the first part
  words = Strings::split(inst, ';');

  // If there is no text, abandon the creation of the Instruction
  if(words.size() == 0) throw std::exception();

  // Split around "->" to see if there is a remote channel specified
  words = Strings::split(words.front(), '>');
  if(words.size() > 1) setRchannel(Strings::strToInt(words[1]));
  else setRchannel(NO_CHANNEL);

  // Split around ' ' to separate all remaining parts of the instruction
  words = Strings::split(words.front(), ' ');

  // Extract information from the opcode
  decodeOpcode(words.front());

  // Determine which registers/immediates any remaining strings represent
  for(unsigned int i=1; i<words.size(); i++) {
    decodeField(words[i], i);
  }

}

Instruction::~Instruction() {

}

/* Private setter methods */
void Instruction::setOp(short val) {
  setBits(startOpcode, startPredicate-1, val);
}

void Instruction::setDest(short val) {
  setBits(startDest, startOpcode-1, val);
}

void Instruction::setSrc1(short val) {
  setBits(startSrc1, startDest-1, val);
}

void Instruction::setSrc2(short val) {
  setBits(startSrc2, startSrc1-1, val);
}

void Instruction::setRchannel(short val) {
  setBits(startChannelID, startSrc2-1, val);
}

void Instruction::setImmediate(int val) {
  setBits(startImmediate, end-1, val);
}

void Instruction::setPredicate(short val) {
  setBits(startPredicate, startSetPred-1, val);
}

void Instruction::setSetPred(bool val) {
  setBits(startSetPred, startSetPred, val?1:0);
}

/* Determine if the string represents a register or immediate, and set the
 * corresponding field. */
void Instruction::decodeField(const string& str, int field) {

  string reg = str;

  if(reg[0] == 'r') {                 // Registers are marked with an 'r'
    reg.erase(0,1);                   // Remove the 'r'
    int value = Strings::strToInt(reg);

    if(field==1) setDest(value);
    else if(field==2) setSrc1(value);
    else if(field==3) setSrc2(value);
  }
  else if(reg[0] == 'c') {            // Channels are optionally marked with "ch"
    reg.erase(0,2);                   // Remove the "ch"

    // There are two reserved registers before channel-ends start
    int value = Registers::fromChannelID(Strings::strToInt(reg));

    if(field==1) setDest(value);
    else if(field==2) setSrc1(value);
    else if(field==3) setSrc2(value);
  }
  // Check that this instruction should have an immediate?
  else setImmediate(Strings::strToInt(reg));

}

/* Extract all information from the opcode, and set the relevant fields. */
void Instruction::decodeOpcode(const string& opcode) {

  // Try splitting the opcode to see if it is predicated
  vector<string> opcodeParts = Strings::split(opcode, '?');

  string opcodeString;

  // Set the predicate bits
  if(opcodeParts.size() > 1) {
    string predicate = opcodeParts[0];
    if(predicate == "p") setPredicate(P);
    else if(predicate == "!p") setPredicate(NOT_P);
    opcodeString = opcodeParts[1];
  }
  else {
    setPredicate(ALWAYS);
    opcodeString = opcodeParts[0];
  }

  // See if the instruction sets the predicate register, or is the end of packet
  opcodeParts = Strings::split(opcodeString, '.');

  if(opcodeParts.size() > 1) {
    string setting = opcodeParts[1];
    if(setting == "eop") setPredicate(END_OF_PACKET);
    else if(setting == "p") setSetPred(true);
    else if(setting == "fetch") {
      string pselfetch = "psel.fetch";
      setOp(InstructionMap::opcode(pselfetch));
      if(opcodeParts.size() > 2 && opcodeParts[2] == "eop")
        setPredicate(END_OF_PACKET);
      setSetPred(false);
      return;
    }
    else setSetPred(false);
  }

  // Look up operation in InstructionMap
  short op = InstructionMap::opcode(opcodeParts.front());
  setOp(op);

}
