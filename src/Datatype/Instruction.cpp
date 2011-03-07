/*
 * Instruction.cpp
 *
 * Current layout: 64 bit value containing:
 *    1 bit saying whether the predicate register should be set
 *    2 predicate bits
 *    7 bit opcode
 *    6 bit destination register location
 *    6 bit source 1 register location
 *    6 bit source 2 register location
 *    4 bit remote channel ID
 *    32 bit immediate
 *
 *  | setpred | pred | opcode | dest | source1 | source2 | channel ID | immed |
 *   63        62     60       53     47        41        35           31    0
 *
 *  Although instructions are currently encoded using 64 bits, they are treated
 *  as though they are only 32 bits long. We eventually intend to have a 32 bit
 *  encoding, but in the meantime, a sparser structure makes modifications and
 *  access easier.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Instruction.h"
#include "DecodedInst.h"
#include "../Exceptions/InvalidInstructionException.h"
#include "../TileComponents/TileComponent.h"
#include "../TileComponents/Pipeline/IndirectRegisterFile.h"
#include "../Utility/InstructionMap.h"
#include "../Utility/StringManipulation.h"
#include "../Utility/Parameters.h"

typedef StringManipulation Strings;
typedef IndirectRegisterFile Registers;

const short startImmediate = 0;
const short startChannelID = startImmediate + 32; // 32
const short startSrc2      = startChannelID + 4;  // 36
const short startSrc1      = startSrc2 + 6;       // 42
const short startDest      = startSrc1 + 6;       // 48
const short startOpcode    = startDest + 6;       // 54
const short startPredicate = startOpcode + 7;     // 61
const short startSetPred   = startPredicate + 2;  // 63
const short end            = startSetPred + 1;    // 64

/* Public getter methods */
uint8_t Instruction::opcode() const {
  return getBits(startOpcode, startPredicate-1);
}

RegisterIndex Instruction::destination() const {
  return getBits(startDest, startOpcode-1);
}

RegisterIndex Instruction::sourceReg1() const {
  return getBits(startSrc1, startDest-1);
}

RegisterIndex Instruction::sourceReg2() const {
  return getBits(startSrc2, startSrc1-1);
}

ChannelIndex Instruction::remoteChannel() const {
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

Word Instruction::firstWord() const {
  return Word(data_ >> 32);
}

Word Instruction::secondWord() const {
  return Word(data_ & 0xFFFFFFFF);
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
  if((inst[0]=='%') || (inst[0]==';') || (inst[0]=='#') || (inst[0]=='\n') || (inst[0]=='\r'))
    throw InvalidInstructionException();

  // Remove any comments by splitting around ';' and only keeping the first part
  vector<string>& words = Strings::split(inst, ';');

  // Should words be deleted before we reassign to it?
  if(words.size() > 0) words = Strings::split(words[0], '#');

  // If there is no text, abandon the creation of the Instruction
  if(words.size() == 0) throw InvalidInstructionException();

  // Split around "->" to see if there is a remote channel specified
  words = Strings::split(words.front(), '>');
  if(words.size() > 1) {
    remoteChannel(decodeRChannel(words[1]));
    words.front().erase(words.front().end()-1); // Remove the "-" from "->"
  }
  else remoteChannel(NO_CHANNEL);

  // Split around ' ' to separate all remaining parts of the instruction
  words = Strings::split(words.front(), ' ');

  // Extract information from the opcode
  decodeOpcode(words.front());

  // Determine which registers/immediates any remaining strings represent
  RegisterIndex reg1 = (words.size() > 1) ? decodeField(words[1]) : 0;
  RegisterIndex reg2 = (words.size() > 2) ? decodeField(words[2]) : 0;
  RegisterIndex reg3 = (words.size() > 3) ? decodeField(words[3]) : 0;

  delete &words;

  // Special case for setchmap because its register and immediate are in a
  // different order.
  if(InstructionMap::operation(opcode()) == InstructionMap::SETCHMAP) {
    reg1 = reg2;
    reg2 = 0;
  }

  setFields(reg1, reg2, reg3);

  // Perform a small check to catch a possible problem.
  string nop("nop");
  if(opcode() == InstructionMap::opcode(nop)) {
    if(reg1 != 0 || reg2 != 0 || reg3 != 0 || remoteChannel() != NO_CHANNEL) {
      cerr << "Warning: possible invalid instruction: " << *this
           << "\n  (generated from " << inst << ")" << endl;
      throw InvalidInstructionException();
    }
  }

}

/* Private setter methods */
void Instruction::opcode(const uint8_t val) {
  setBits(startOpcode, startPredicate-1, val);
}

void Instruction::destination(const RegisterIndex val) {
  setBits(startDest, startOpcode-1, val);
}

void Instruction::sourceReg1(const RegisterIndex val) {
  setBits(startSrc1, startDest-1, val);
}

void Instruction::sourceReg2(const RegisterIndex val) {
  setBits(startSrc2, startSrc1-1, val);
}

void Instruction::remoteChannel(const ChannelIndex val) {
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
RegisterIndex Instruction::decodeField(const string& str) {

  string reg = str;

  if(reg[0] == 'r') {                 // Registers are marked with an 'r'
    reg.erase(0,1);                   // Remove the 'r'
    int value = Strings::strToInt(reg);

    return value;
  }

  if(reg[0] == 'c') {                 // Channels are optionally marked with "ch"
    reg.erase(0,2);                   // Remove the "ch"

    ChannelIndex channel = Strings::strToInt(reg);
    assert(channel < NUM_RECEIVE_CHANNELS);

    // Convert from the channel index to the register index.
    RegisterIndex value = Registers::fromChannelID(channel);

    return value;
  }

  vector<string>& parts = Strings::split(reg, '(');

  if(parts.size() > 1) {              // Dealing with load/store notation: 8(r2)
    immediate(Strings::strToInt(parts[0]));

    parts[1].erase(parts[1].end()-1); // Erase the closing bracket.
    RegisterIndex result = decodeField(parts[1]);

    delete &parts;
    return result;
  }

  delete &parts;

  // Check that this instruction should have an immediate?

  // Use decodeRChannel to allow the immediate to be written in remote
  // channel notation: (12,2).
  immediate(decodeRChannel(reg));
  return 0;

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

  // Does opcodeParts need deleting here, before we assign to it again?
//  delete &opcodeParts;

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
  try {
    short op = InstructionMap::opcode(opcodeParts.front());
    opcode(op);
  }
  catch(std::exception& e) {
    // Not a valid operation name.
    if(DEBUG) cerr << "Error: invalid operation name: " << opcodeParts.front() << endl;
    throw InvalidInstructionException();
  }

  delete &opcodeParts;

}

/* Set the remote channel field depending on the contents of the given string.
 * The string may contain an integer, or a pair of integers in the form (x,y),
 * representing a component and one of its input ports.
 * Returns a signed int because this method is also used to decode immediates. */
int32_t Instruction::decodeRChannel(const string& channel) {
  vector<string>& parts = Strings::split(channel, ',');
  ChannelID channelID;

  if(parts.size() == 1) {
    channelID = Strings::strToInt(parts[0]);
  }
  else {
    // We should now have two strings of the form "(x" and "y)".
    assert(parts.size()==2);

    parts[0].erase(parts[0].begin());           // Remove the bracket
    parts[1].erase(parts[1].end()-1);           // Remove the bracket
    channelID = TileComponent::inputPortID(Strings::strToInt(parts[0]),
                                           Strings::strToInt(parts[1]));
  }

  delete &parts;
  return channelID;
}

void Instruction::setFields(const RegisterIndex reg1, const RegisterIndex reg2,
                            const RegisterIndex reg3) {
  int operation = InstructionMap::operation(opcode());

  switch(operation) {

    // Single source, no destination.
    case InstructionMap::STWADDR :
    case InstructionMap::STBADDR :
    case InstructionMap::RMTFETCH :
    case InstructionMap::RMTFETCHPST :
    case InstructionMap::RMTFILL : {
      sourceReg1(reg1);
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
  // It is easier to print from DecodedInst, as it has all of the fields
  // separated.
  os << DecodedInst(*this);
  return os;
}
