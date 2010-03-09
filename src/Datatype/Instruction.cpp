/*
 * Instruction.cpp
 *
 * Current layout: 64 bit value containing:
 *    16 bit immediate
 *    1 bit saying whether the predicate register should be set
 *    2 predicate bits
 *    7 bit opcode
 *    5 bit destination register location
 *    5 bit source 1 register location
 *    5 bit source 2 register location
 *    8 bit remote channel ID
 *
 *    | immed | setpred | pred | opcode | dest | source1 | source2 | channel ID |
 *     48      32        31     29       22     17        12        7          0
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Instruction.h"
#include "../Utility/InstructionMap.h"

const short startImmediate = 33;
const short startSetPred = 32;
const short startPredicate = 30;
const short startOpcode = 23;
const short startDest = 18;
const short startSrc1 = 13;
const short startSrc2 = 8;
const short startChannelID = 0;

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

signed short Instruction::getImmediate() const {
  return getBits(startImmediate, 48);
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

Instruction::Instruction(const string &inst) {

  vector<string> words;

  // Remove any comments by splitting around ';' and only keeping the first part
  words = split(inst, ';');

  // Split around "->" to see if there is a remote channel specified
  words = split(words.front(), '>');
  if(words.size() > 1) setRchannel(strToInt(words[1]));

  // Split around ' ' to separate all remaining parts of the instruction
  words = split(words.front(), ' ');

  // Try splitting the opcode to see if it is predicated
  vector<string> opcodeParts = split(words.front(), '?');

  string opcodeString;

  // Set the predicate bits
  if(opcodeParts.size() > 1) {
    string predicate = opcodeParts[0];
    if(predicate == "p") setPredicate(P);
    else if(predicate == "!p") setPredicate(NOT_P);
//    else if(predicate == "eop") setPredicate(END_OF_PACKET);
    opcodeString = opcodeParts[1];
  }
  else {
    setPredicate(ALWAYS);
    opcodeString = opcodeParts[0];
  }

  // See if the instruction sets the predicate register, or is the end of packet
  opcodeParts = split(opcodeString, '.');

  if(opcodeParts.size() > 1) {
    string setting = opcodeParts[1];
    if(setting == "eop") setPredicate(END_OF_PACKET);
    else if(setting == "p") setSetPred(true);
    else setSetPred(false);
  }

  // Look up operation in InstructionMap
  short opcode = InstructionMap::opcode(opcodeParts.front());
  setOp(opcode);

  // Convert all remaining strings to integers, and set the appropriate fields
  for(unsigned int i=1; i<words.size(); i++) {
    decode(words[i], i);
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

void Instruction::setImmediate(short val) {
  setBits(startImmediate, 48, val);
}

void Instruction::setPredicate(short val) {
  setBits(startPredicate, startSetPred-1, val);
}

void Instruction::setSetPred(bool val) {
  setBits(startSetPred, startSetPred, val?1:0);
}

/* String manipulation methods -- move to a separate file? */

/* Split a string around a given delimiter character */
vector<string>& Instruction::split(const string &s, char delim,
                                   vector<string> &elems) {
  std::stringstream ss(s);
  string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

/* Convenience method for the method above */
vector<string> Instruction::split(const string &s, char delim) {
  vector<string> elems;
  return split(s, delim, elems);
}

/* Return the integer represented by the given string */
int Instruction::strToInt(const string &str) {
  std::stringstream ss(str);
  int num;
  ss >> num;
  return num;
}

/* Determine if the string represents a register or immediate, and set the
 * corresponding field. */
void Instruction::decode(const string &str, int field) {
  string reg = str;

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
