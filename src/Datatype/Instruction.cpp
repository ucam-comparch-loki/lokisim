/*
 * Instruction.cpp
 *
 * Current layout: 64 bit value containing:
 *    1 bit saying whether the predicate register should be set
 *    2 predicate bits
 *    6 bit opcode
 *    5 bit destination register location
 *    5 bit source 1 register location
 *    5 bit source 2 register location
 *    8 bit remote channel ID
 *    32 bit immediate
 *
 *    | setpred | pred | opcode | dest | source1 | source2 | channel ID | immed |
 *     63        62     60       54     49        44        39           31    0
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Instruction.h"
#include "../Exceptions/InvalidInstructionException.h"
#include "../Utility/InstructionMap.h"
#include "../Utility/StringManipulation.h"
#include "../Utility/Parameters.h"
#include "../TileComponents/Pipeline/IndirectRegisterFile.h"

typedef StringManipulation Strings;
typedef IndirectRegisterFile Registers;

const short startImmediate = 0;
const short startChannelID = startImmediate + 32; // 32
const short startSrc2      = startChannelID + 8;  // 40
const short startSrc1      = startSrc2 + 5;       // 45
const short startDest      = startSrc1 + 5;       // 50
const short startOpcode    = startDest + 5;       // 55
const short startPredicate = startOpcode + 6;     // 61
const short startSetPred   = startPredicate + 2;  // 63
const short end            = startSetPred + 1;    // 64

/* Public getter methods */
uint8_t Instruction::opcode() const {
  return getBits(startOpcode, startPredicate-1);
}

uint8_t Instruction::destination() const {
  return getBits(startDest, startOpcode-1);
}

uint8_t Instruction::sourceReg1() const {
  return getBits(startSrc1, startDest-1);
}

uint8_t Instruction::sourceReg2() const {
  return getBits(startSrc2, startSrc1-1);
}

uint8_t Instruction::remoteChannel() const {
  return getBits(startChannelID, startSrc2-1);
}

int32_t Instruction::immediate() const {
  return (int32_t)getBits(startImmediate, startChannelID-1);
}

uint8_t Instruction::predicate() const {
  return getBits(startPredicate, startSetPred-1);
}

bool Instruction::setsPredicate() const {
  return getBits(startSetPred, end-1);
}

bool Instruction::endOfPacket() const {
  return predicate() == END_OF_PACKET;
}


uint64_t Instruction::toLong() const {
  return data_;
}

bool Instruction::operator== (const Instruction& other) const {
  return this->data_ == other.data_;
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
  opcode(InstructionMap::opcode(InstructionMap::name(opcode())));
}

Instruction::Instruction(const string& inst) {

  // Skip this line if it is a comment or empty
  if((inst[0]=='%') || (inst[0]==';') || (inst[0]=='\n') || (inst[0]=='\r'))
    throw InvalidInstructionException();

  // Remove any comments by splitting around ';' and only keeping the first part
  vector<string>& words = Strings::split(inst, ';');

  // If there is no text, abandon the creation of the Instruction
  if(words.size() == 0) throw InvalidInstructionException();

  // Split around "->" to see if there is a remote channel specified
  words = Strings::split(words.front(), '>');
  if(words.size() > 1) remoteChannel(decodeRChannel(words[1]));
  else remoteChannel(NO_CHANNEL);

  // Split around ' ' to separate all remaining parts of the instruction
  words = Strings::split(words.front(), ' ');

  // Extract information from the opcode
  decodeOpcode(words.front());

  // Determine which registers/immediates any remaining strings represent
  uint8_t reg1 = (words.size() > 1) ? decodeField(words[1]) : 0;
  uint8_t reg2 = (words.size() > 2) ? decodeField(words[2]) : 0;
  uint8_t reg3 = (words.size() > 3) ? decodeField(words[3]) : 0;
  setFields(reg1, reg2, reg3);

  delete &words;

  // Perform a small check to catch a possible problem.
  string nop("nop");
  if(opcode() == InstructionMap::opcode(nop)) {
    if(reg1 != 0 || reg2 != 0 || reg3 != 0 || remoteChannel() != NO_CHANNEL) {
      cerr << "Warning: possible invalid instruction: " << *this
           << "\n  (generated from " << inst << ")" << endl;
    }
  }

}

Instruction::~Instruction() {

}

/* Private setter methods */
void Instruction::opcode(const uint8_t val) {
  setBits(startOpcode, startPredicate-1, val);
}

void Instruction::destination(const uint8_t val) {
  setBits(startDest, startOpcode-1, val);
}

void Instruction::sourceReg1(const uint8_t val) {
  setBits(startSrc1, startDest-1, val);
}

void Instruction::sourceReg2(const uint8_t val) {
  setBits(startSrc2, startSrc1-1, val);
}

void Instruction::remoteChannel(const uint8_t val) {
  setBits(startChannelID, startSrc2-1, val);
}

void Instruction::immediate(const int32_t val) {
  setBits(startImmediate, startChannelID-1, val);
}

void Instruction::predicate(const uint8_t val) {
  setBits(startPredicate, startSetPred-1, val);
}

void Instruction::setsPredicate(const bool val) {
  setBits(startSetPred, end-1, val?1:0);
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
    immediate(decodeRChannel(reg));
    return 0;
  }

}

/* Extract all information from the opcode, and set the relevant fields. */
void Instruction::decodeOpcode(const string& name) {

  // Try splitting the opcode to see if it is predicated
  vector<string>& opcodeParts = Strings::split(name, '?');

  string opcodeString;

  // Set the predicate bits
  if(opcodeParts.size() > 1) {
    string pred = opcodeParts[0];
    if(pred == "p" || pred == "ifp")        predicate(P);
    else if(pred == "!p" || pred == "if!p") predicate(NOT_P);
    opcodeString = opcodeParts[1];
  }
  else {
    predicate(ALWAYS);
    opcodeString = opcodeParts[0];
  }

  // See if the instruction sets the predicate register, or is the end of packet
  opcodeParts = Strings::split(opcodeString, '.');

  if(opcodeParts.size() > 1) {
    string setting = opcodeParts[1];
    if(setting == "eop") predicate(END_OF_PACKET);
    else if(setting == "p") setsPredicate(true);
    else if(setting == "fetch") {
      string pselfetch = "psel.fetch";
      opcode(InstructionMap::opcode(pselfetch));
      if(opcodeParts.size() > 2 && opcodeParts[2] == "eop")
        predicate(END_OF_PACKET);
      setsPredicate(false);
      return;
    }
    else setsPredicate(false);
  }

  // Look up operation in InstructionMap
  short op = InstructionMap::opcode(opcodeParts.front());
  opcode(op);

  delete &opcodeParts;

}

/* Set the remote channel field depending on the contents of the given string.
 * The string may contain an integer, or a pair of integers in the form (x,y),
 * representing a component and one of its input ports.
 * Returns a signed int because this method is also used to decode immediates. */
int32_t Instruction::decodeRChannel(const string& channel) {
  vector<string>& parts = Strings::split(channel, ',');
  if(parts.size() == 1) return Strings::strToInt(parts[0]);
  else {
    string component = Strings::split(parts[0],'(').at(1); // Remove the bracket
    string port = Strings::split(parts[1],')').at(0);      // Remove the bracket
    int channel = Strings::strToInt(component) * NUM_CLUSTER_INPUTS
                + Strings::strToInt(port);
    return channel;
  }

  delete &parts;
}

void Instruction::setFields(const uint8_t reg1, const uint8_t reg2,
                            const uint8_t reg3) {
  int operation = InstructionMap::operation(opcode());

  switch(operation) {

    // Single source, no destination.
    case InstructionMap::LDW :
    case InstructionMap::LDBU :
    case InstructionMap::STWADDR :
    case InstructionMap::STBADDR :
    case InstructionMap::FETCH :
    case InstructionMap::FETCHPST :
    case InstructionMap::RMTFETCH :
    case InstructionMap::RMTFETCHPST :
    case InstructionMap::RMTFILL :
    case InstructionMap::SETCHMAP : {
      sourceReg1(reg1);
      break;
    }

    // Two sources, no destination.
    case InstructionMap::STW :
    case InstructionMap::STB :
    case InstructionMap::PSELFETCH : {
      sourceReg1(reg1);
      sourceReg2(reg2);
      break;
    }

    // Two sources and a destination.
    default : {
      destination(reg1);
      sourceReg1(reg2);
      sourceReg2(reg3);
      break;
    }
  }
}

std::ostream& Instruction::print(std::ostream& os) const {
  if(predicate() == P) os << "p?";
  else if(predicate() == NOT_P) os << "!p?";

  os << InstructionMap::name(InstructionMap::operation(opcode()))
     << (setsPredicate()?".p":"")  << (endOfPacket()?".eop":"")
     << " r" << (int)destination() << " r" << (int)sourceReg1()
     << " r" << (int)sourceReg2()  << " "  << immediate();
  if(remoteChannel() != NO_CHANNEL) os << " -> " << (int)remoteChannel();
  return os;
}
