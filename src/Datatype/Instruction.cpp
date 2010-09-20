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
#include "../Exceptions/InvalidInstructionException.h"
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
uint8_t Instruction::getOp() const {
  return getBits(startOpcode, startPredicate-1);
}

uint8_t Instruction::getDest() const {
  return getBits(startDest, startOpcode-1);
}

uint8_t Instruction::getSrc1() const {
  return getBits(startSrc1, startDest-1);
}

uint8_t Instruction::getSrc2() const {
  return getBits(startSrc2, startSrc1-1);
}

uint8_t Instruction::getRchannel() const {
  return getBits(startChannelID, startSrc2-1);
}

int32_t Instruction::getImmediate() const {
  return getBits(startImmediate, end-1);
}

uint8_t Instruction::getPredicate() const {
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

Instruction::Instruction(const uint64_t inst) : Word(inst) {
  // Need to change the opcode representation in case it has changed
  // internally.
  setOp(InstructionMap::opcode(InstructionMap::name(getOp())));
}

Instruction::Instruction(const string& inst) {

  vector<string> words;

  // Skip this line if it is a comment or empty
  if((inst[0]=='%') || (inst[0]==';') || (inst[0]=='\n') || (inst[0]=='\r'))
    throw InvalidInstructionException();

  // Remove any comments by splitting around ';' and only keeping the first part
  words = Strings::split(inst, ';');

  // If there is no text, abandon the creation of the Instruction
  if(words.size() == 0) throw InvalidInstructionException();

  // Split around "->" to see if there is a remote channel specified
  words = Strings::split(words.front(), '>');
  if(words.size() > 1) setRchannel(decodeRChannel(words[1]));
  else setRchannel(NO_CHANNEL);

  // Split around ' ' to separate all remaining parts of the instruction
  words = Strings::split(words.front(), ' ');

  // Extract information from the opcode
  decodeOpcode(words.front());

  // Determine which registers/immediates any remaining strings represent
  uint8_t reg1 = (words.size() > 1) ? decodeField(words[1]) : 0;
  uint8_t reg2 = (words.size() > 2) ? decodeField(words[2]) : 0;
  uint8_t reg3 = (words.size() > 3) ? decodeField(words[3]) : 0;
  setFields(reg1, reg2, reg3);

}

Instruction::~Instruction() {

}

/* Private setter methods */
void Instruction::setOp(const uint8_t val) {
  setBits(startOpcode, startPredicate-1, val);
}

void Instruction::setDest(const uint8_t val) {
  setBits(startDest, startOpcode-1, val);
}

void Instruction::setSrc1(const uint8_t val) {
  setBits(startSrc1, startDest-1, val);
}

void Instruction::setSrc2(const uint8_t val) {
  setBits(startSrc2, startSrc1-1, val);
}

void Instruction::setRchannel(const uint8_t val) {
  setBits(startChannelID, startSrc2-1, val);
}

void Instruction::setImmediate(const int32_t val) {
  setBits(startImmediate, end-1, val);
}

void Instruction::setPredicate(const uint8_t val) {
  setBits(startPredicate, startSetPred-1, val);
}

void Instruction::setSetPred(const bool val) {
  setBits(startSetPred, startSetPred, val?1:0);
}

/* Determine which register the given string represents. It may be simply a
 * case of extracting the integer shown (e.g. "r5"), or an offset may need to
 * be applied ("ch2"), or the string may represent an immediate. In the case
 * of an immediate, 0 is returned, and the immediate field is set. */
uint8_t Instruction::decodeField(const string& str) {

  string reg = str;

  if(reg[0] == 'r') {                 // Registers are marked with an 'r'
    reg.erase(0,1);                   // Remove the 'r'
    int value = Strings::strToInt(reg);

    return value;
  }
  else if(reg[0] == 'c') {            // Channels are optionally marked with "ch"
    reg.erase(0,2);                   // Remove the "ch"

    // There are two reserved registers before channel-ends start
    int value = Registers::fromChannelID(Strings::strToInt(reg));

    return value;
  }
  // Check that this instruction should have an immediate?
  else {
    // Use decodeRChannel to allow the immediate to be written in remote
    // channel notation: (12,2).
    // TODO: doesn't work yet because we split around commas earlier.
    setImmediate(decodeRChannel(reg));
    return 0;
  }

}

/* Extract all information from the opcode, and set the relevant fields. */
void Instruction::decodeOpcode(const string& opcode) {

  // Try splitting the opcode to see if it is predicated
  vector<string> opcodeParts = Strings::split(opcode, '?');

  string opcodeString;

  // Set the predicate bits
  if(opcodeParts.size() > 1) {
    string predicate = opcodeParts[0];
    if(predicate == "p" || predicate == "ifp") setPredicate(P);
    else if(predicate == "!p" || predicate == "if!p") setPredicate(NOT_P);
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

/* Set the remote channel field depending on the contents of the given string.
 * The string may contain an integer, or a pair of integers in the form (x,y),
 * representing a component and one of its input ports. */
uint8_t Instruction::decodeRChannel(const string& channel) {
  vector<string> parts = Strings::split(channel, ',');
  if(parts.size() == 1) return Strings::strToInt(parts[0]);
  else {
    string component = Strings::split(parts[0],'(').at(1); // Remove the bracket
    string port = Strings::split(parts[1],')').at(0);      // Remove the bracket
    int channel = Strings::strToInt(component) * NUM_CLUSTER_INPUTS
                + Strings::strToInt(port);
    return channel;
  }
}

void Instruction::setFields(const uint8_t reg1, const uint8_t reg2,
                            const uint8_t reg3) {
  int operation = InstructionMap::operation(getOp());

  switch(operation) {

    // Single source, no destination.
    case InstructionMap::LD :
    case InstructionMap::LDB :
    case InstructionMap::STADDR :
    case InstructionMap::STBADDR :
    case InstructionMap::FETCH :
    case InstructionMap::FETCHPST :
    case InstructionMap::RMTFETCH :
    case InstructionMap::RMTFETCHPST :
    case InstructionMap::RMTFILL :
    case InstructionMap::SETCHMAP : {
      setSrc1(reg1);
      break;
    }

    // Two sources, no destination.
    case InstructionMap::ST :
    case InstructionMap::STB :
    case InstructionMap::PSELFETCH : {
      setSrc1(reg1);
      setSrc2(reg2);
      break;
    }

    // Two sources and a destination.
    default : {
      setDest(reg1);
      setSrc1(reg2);
      setSrc2(reg3);
      break;
    }
  }
}
