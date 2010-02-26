/*
 * Instruction.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Instruction.h"
#include "../Utility/InstructionMap.h"

/*
 * Current (fixed) layout: 64 bit value containing:
 *    32 bit immediate (don't need this much, but nothing else is using the space)
 *    2 predicate bits
 *    7 bit opcode
 *    5 bit destination register location
 *    5 bit source 1 register location
 *    5 bit source 2 register location
 *    8 bit remote channel ID
 *
 *    | immed | pred | opcode | dest | source1 | source2 | channel ID |
 *     63      31     29       22     17        12        7          0
 *
 */

const short startImmediate = 32;
const short startPredicate = 30;
const short startOpcode = 23;
const short startDest = 18;
const short startSrc1 = 13;
const short startSrc2 = 8;

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
  return getBits(0, startSrc2-1);
}

unsigned int Instruction::getImmediate() const {
  return getBits(startImmediate, 63);
}

unsigned short Instruction::getPredicate() const {
  return getBits(startPredicate, startImmediate-1);
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

Instruction::Instruction(const std::string &inst) {

  std::vector<std::string> words;

  // Remove any comments by splitting around ';' and only keeping the first part
  words = split(inst, ';');

  // Split around "->" to see if there is a remote channel specified
  words = split(words.front(), '>');
  if(words.size() > 1) setRchannel(strToInt(words.at(1)));

  // Split around ' ' to separate all remaining parts of the instruction
  words = split(words.front(), ' ');

  // Try splitting the opcode to see if it is predicated
  std::vector<std::string> opcodeParts = split(words.front(), '.');

  // Look up operation in InstructionMap
  short opcode = InstructionMap::opcode(opcodeParts.front());
  setOp(opcode);

  // Set the predicate bits
  if(opcodeParts.size() > 1) {
    std::string predicate = opcodeParts.at(1);
    if(predicate == "p") setPredicate(P);
    else if(predicate == "!p") setPredicate(NOT_P);
    else if(predicate == "eop") setPredicate(END_OF_PACKET);
  }
  else setPredicate(ALWAYS);

  // Convert all remaining strings to integers, and set the appropriate fields
  for(unsigned int i=1; i<words.size(); i++) {
    decode(words.at(i), i);
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
  setBits(0, startSrc2-1, val);
}

void Instruction::setImmediate(unsigned int val) {
  setBits(startImmediate, 63, val);
}

void Instruction::setPredicate(short val) {
  setBits(startPredicate, startImmediate-1, val);
}

/* String manipulation methods -- move to a separate file? */

/* Split a string around a given delimiter character */
std::vector<std::string>& Instruction::split(const std::string &s, char delim,
                                             std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

/* Convenience method for the method above */
std::vector<std::string> Instruction::split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return split(s, delim, elems);
}

/* Return the integer represented by the given string */
int Instruction::strToInt(const std::string &str) {
  std::stringstream ss(str);
  int num;
  ss >> num;
  return num;
}

/* Determine if the string represents a register or immediate, and set the
 * corresponding field. */
void Instruction::decode(const std::string &str, int field) {
  std::string reg = str;

  if(reg[0] == 'r') {                 // Registers are marked with an 'r'
    reg.erase(0,1);                   // Remove the 'r'
    int value = strToInt(reg);

    if(field==1) setDest(value);
    else if(field==2) setSrc1(value);
    else if(field==3) setSrc2(value);
  }
  // Check that this instruction should have an immediate?
  else setImmediate(strToInt(reg));

}
