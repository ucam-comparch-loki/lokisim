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
#include "../TileComponents/Pipeline/RegisterFile.h"
#include "../Utility/InstructionMap.h"
#include "../Utility/StringManipulation.h"
#include "../Utility/Parameters.h"

typedef StringManipulation Strings;
typedef RegisterFile Registers;

const short startPredicate = 30, endPredicate = 31;
const short startOpcode    = 23, endOpcode    = 29;
const short startReg1      = 18, endReg1      = 22;
const short startReg2      =  9, endReg2      = 13;
const short startReg3      =  4, endReg3      =  8;
const short startChannel   = 14, endChannel   = 17;
const short startFunction  =  0, endFunction  =  3;

// Return the entire word as the immediate: the decoding chooses which part is
// relevant.
const short startImmediate =  0, endImmediate = 31;


/* Public getter methods */
opcode_t Instruction::opcode() const {
  return (opcode_t)getBits(startOpcode, endOpcode);
}

function_t Instruction::function() const {
  return (function_t)getBits(startFunction, endFunction);
}

RegisterIndex Instruction::reg1() const {
  return getBits(startReg1, endReg1);
}

RegisterIndex Instruction::reg2() const {
  return getBits(startReg2, endReg2);
}

RegisterIndex Instruction::reg3() const {
  return getBits(startReg3, endReg3);
}

ChannelIndex Instruction::remoteChannel() const {
  return getBits(startChannel, endChannel);
}

int32_t Instruction::immediate() const {
  return (int32_t)getBits(startImmediate, endImmediate);
}

predicate_t Instruction::predicate() const {
  return (predicate_t)getBits(startPredicate, endPredicate);
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
  // Do nothing
}

Instruction::Instruction(const string& inst) : Word() {
  // Skip this line if it is a comment or empty
  if((inst[0]=='%') || (inst[0]==';') || (inst[0]=='#') || (inst[0]=='\n') || (inst[0]=='\r'))
    throw InvalidInstructionException();

  // Remove comments - split around ';' and only keep the first part
  vector<string>& words = Strings::split(inst, ';');
  string word;

  // Should words be deleted before we reassign to it?
  if(words.size() > 0) {
    word = words[0];
    delete &words;
    words = Strings::split(word, '#');
  }

  // If there is no text, abandon the creation of the Instruction
  if(words.size() == 0) {
    delete &words;
    throw InvalidInstructionException();
  }

  // Remote channel - split around "->"
  word = words.front();
  delete &words;
  words = Strings::split(word, '>');
  if(words.size() > 1) {
    remoteChannel(decodeImmediate(words[1]));
    words.front().erase(words.front().end()-1); // Remove the "-" from "->"
  }
  else remoteChannel(NO_CHANNEL);

  // Split around ' ' to separate all remaining parts of the instruction
  word = words.front();
  delete &words;
  words = Strings::split(word, ' ');

  // Opcode
  decodeOpcode(words.front());

  // Registers/immediates
  RegisterIndex reg1 = (words.size() > 1) ? decodeField(words[1]) : 0;
  RegisterIndex reg2 = (words.size() > 2) ? decodeField(words[2]) : 0;
  RegisterIndex reg3 = (words.size() > 3) ? decodeField(words[3]) : 0;

  delete &words;

  // Special case for setchmap because its register and immediate
  // are in a different order.
  if(opcode() == InstructionMap::OP_SETCHMAP ||
     opcode() == InstructionMap::OP_SETCHMAPI) {
    reg1 = reg2;
    reg2 = 0;
  }

  setFields(reg1, reg2, reg3);

}

/* Private setter methods */
void Instruction::opcode(const opcode_t val) {
  setBits(startOpcode, endOpcode, (int)val);
}

void Instruction::function(const function_t val) {
  setBits(startFunction, endFunction, (int)val);
}

void Instruction::reg1(const RegisterIndex val) {
  assert(val < NUM_ADDRESSABLE_REGISTERS);
  setBits(startReg1, endReg1, val);
}

void Instruction::reg2(const RegisterIndex val) {
  assert(val < NUM_ADDRESSABLE_REGISTERS);
  setBits(startReg2, endReg2, val);
}

void Instruction::reg3(const RegisterIndex val) {
  assert(val < NUM_ADDRESSABLE_REGISTERS);
  setBits(startReg3, endReg3, val);
}

void Instruction::remoteChannel(const ChannelIndex val) {
  assert(val == NO_CHANNEL || val < CHANNEL_MAP_SIZE);
  setBits(startChannel, endChannel, val);
}

void Instruction::immediate(const int32_t val) {
  // Immediates have different lengths, depending on the instruction type.
  int length = 0;
  switch(InstructionMap::format(opcode())) {
    case InstructionMap::FMT_FF:   length = 23; break;

    case InstructionMap::FMT_0R:
    case InstructionMap::FMT_0Rnc:
    case InstructionMap::FMT_1R:   length = 14; break;

    case InstructionMap::FMT_1Rnc: length = 16; break;

    case InstructionMap::FMT_2R:
    case InstructionMap::FMT_2Rnc: length = 9; break;

    case InstructionMap::FMT_2Rs:  length = 5; break;

    case InstructionMap::FMT_3R:   length = 0; break;
  }

  setBits(startImmediate, length-1, val);
}

void Instruction::predicate(const predicate_t val) {
  setBits(startPredicate, endPredicate, (int)val);
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

  // Use decodeImmediate to allow the immediate to be written in remote
  // channel notation: (12,2).
  immediate(decodeImmediate(reg));
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

  delete &opcodeParts;

  // See if the instruction sets the predicate register, or is the end of packet
  opcodeParts = Strings::split(opcodeString, '.');
  string opname = opcodeParts.front();

  if(opcodeParts.size() > 1) {
    string setting = opcodeParts[1];
    if(setting == "eop") predicate(END_OF_PACKET);
    else if(setting == "fetch") {
      opcode(InstructionMap::OP_PSEL_FETCH);
      if(opcodeParts.size() > 2 && opcodeParts[2] == "eop")
        predicate(END_OF_PACKET);

      delete &opcodeParts;
      return;
    }
    else if(setting == "p") {
      // ".p" modes are now included as separate opcodes.
      opname = opcodeString;
    }
  }

  // If the "opcode" is an assembly label.
  if(opname[opname.length()-1] == ':') {
    delete &opcodeParts;
    throw InvalidInstructionException();
  }

  // Look up operation in InstructionMap
  try {
    opcode_t op = InstructionMap::opcode(opname);
    opcode(op);

    if(op == 0 || op == 1) {
      function_t fn = InstructionMap::function(opname);
      function(fn);
    }
  }
  catch(std::exception& e) {
    // Not a valid operation name (and not a label).
    if(DEBUG) cerr << "Error: invalid operation name: " << opname << endl;
    delete &opcodeParts;
    throw InvalidInstructionException();
  }

  delete &opcodeParts;

}

/* Parse a number of possible notations for immediates.
 * The string may contain:
 *  - an integer,
 *  - a tuple in the form (mxxxxxxxx, y), representing a multicast address and
 *    the input channel to use,
 *  - a triple of integers in the form (x,y,z), representing a component and
 *    one of its input channels,
 *  - a 5-tuple (x,y,z,s,t) referencing a virtual memory group. */
int32_t Instruction::decodeImmediate(const string& immed) {
	vector<string>& parts = Strings::split(immed, ',');
	int32_t value;

	if (parts.size() == 1) {
	  // Plain integer - just parse it.
		value = Strings::strToInt(parts[0]);
	} else if (parts.size() == 2) {
	  // This should be a multicast address - no tile ID is needed.
	  // Strings should be of the form "(m01010110", and "channel)"

	  assert(parts[0][1] == 'm');

    parts[0].erase(0,2);                        // Remove the bracket and the m
    parts[1].erase(parts[1].end()-1);           // Remove the bracket

    assert(parts[0].length() == CORES_PER_TILE);

    int mcastAddress = 0;
    int channel = Strings::strToInt(parts[1]);

    for(unsigned int i=0; i < CORES_PER_TILE; i++) {
      int shiftAmount = CORES_PER_TILE - i - 1;
      if(parts[0][i] == '1') mcastAddress |= (1 << shiftAmount);
    }

    value = ChannelID(0, mcastAddress, channel, true).getData();
	} else if (parts.size() == 3) {

		// We should now have strings of the form "(x", "y" and "z)"

		parts[0].erase(parts[0].begin());           // Remove the bracket
		parts[2].erase(parts[2].end()-1);           // Remove the bracket

		int tile = Strings::strToInt(parts[0]);
		int position = Strings::strToInt(parts[1]);
		int channelIndex = Strings::strToInt(parts[2]);

		ComponentID checkID(tile, position);
		assert(checkID.isCore() && channelIndex < (int)CORE_INPUT_CHANNELS);

		value = ChannelID(tile, position, channelIndex).getData();
	} else if (parts.size() == 5) {
		// We should now have strings of the form "(x", "y", "z", "s" and "t)"

		parts[0].erase(parts[0].begin());           // Remove the bracket
		parts[4].erase(parts[4].end()-1);           // Remove the bracket

		int tile = Strings::strToInt(parts[0]);
		int position = Strings::strToInt(parts[1]);
		int channelIndex = Strings::strToInt(parts[2]);
		int groupBits = Strings::strToInt(parts[3]);
		int lineBits = Strings::strToInt(parts[4]);

		// I find it useful to add together incomplete tuples, which this assertion
		// doesn't allow.
		// e.g. (0,8,0,3,6) + (0,0,id,0,0)
//		ComponentID checkID(tile, position);
//		assert(checkID.isMemory() && channelIndex < (int)MEMORY_CHANNEL_MAP_TABLE_ENTRIES);

		// Encoded entry format:
		// | Tile : 12 | Position : 8 | Channel : 4 | Memory group bits : 4 | Memory line bits : 4 |

		value = ChannelID(tile, position, channelIndex).getData() | (groupBits << 4) | lineBits;
	} else {
		// Invalid format
	  cerr << "Error: invalid tuple length: " << immed << endl;
	  delete &parts;
		assert(false);
	}

	delete &parts;
	return value;
}

void Instruction::setFields(const RegisterIndex val1, const RegisterIndex val2,
                            const RegisterIndex val3) {

  switch(InstructionMap::format(opcode())) {
    case InstructionMap::FMT_1R:
    case InstructionMap::FMT_1Rnc:
      reg1(val1);
      break;

    case InstructionMap::FMT_2R:
    case InstructionMap::FMT_2Rnc:
    case InstructionMap::FMT_2Rs:
      reg1(val1);
      reg2(val2);
      break;

    case InstructionMap::FMT_3R:
      reg1(val1);
      reg2(val2);
      reg3(val3);
      break;

    default:
      break;
  }
}

std::ostream& Instruction::print(std::ostream& os) const {
  // It is easier to print from DecodedInst, as it has all of the fields
  // separated.
  os << DecodedInst(*this);
  return os;
}
