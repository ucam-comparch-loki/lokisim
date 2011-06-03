/*
 * ELFFileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include <stdio.h>

#include "ELFFileReader.h"
#include "DataBlock.h"
#include "../Debugger.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/ChannelID.h"
#include "../../Datatype/Instruction.h"
#include "../../TileComponents/Cluster.h"

vector<DataBlock>& ELFFileReader::extractData(int& mainPos) const {
  vector<DataBlock>* blocks = new vector<DataBlock>();

  // Open the input file.
  std::ifstream elfFile(filename_.c_str());

  string extFileName = filename_ + ".objdump-h";
  bool useExternalMetadata = access(extFileName.c_str(), F_OK) == 0;

  // Execute a command which returns information on all of the ELF sections.
  FILE* terminalOutput;

  if (useExternalMetadata) {
	  terminalOutput = fopen(extFileName.c_str(), "r");
  } else {
	  string command("loki-elf-objdump -h " + filename_);
	  terminalOutput = popen(command.c_str(), "r");
  }

  char line[100];

  // Step through each line of information, looking for ones of interest.
  // fgets only returns a word, not a whole line?
  while(fgets(line, 100, terminalOutput) != NULL) {
    string lineStr(line);
    vector<string>& words = StringManipulation::split(lineStr, ' ');

    // We're only interested in a few particular lines of the information.
    if(words.size() == 7 && (words[1]==".text" || words[1]==".data" || words[1]==".rodata")) {
      string name      = words[1];
      int size         = StringManipulation::strToInt("0x"+words[2]);
//      int virtPosition = StringManipulation::strToInt("0x"+words[3]);
      int physPosition = StringManipulation::strToInt("0x"+words[4]);
      int offset       = StringManipulation::strToInt("0x"+words[5]);

      // Seek to "offset" in elfFile.
      elfFile.seekg(offset);

      vector<Word>* data = new vector<Word>();

      if(name == ".text") {
        // Read in "size" bytes (remembering that each instruction is 8 bytes).
        for(int i=0; i<size; i+=BYTES_PER_INSTRUCTION) {
          Instruction inst = nextWord(elfFile, true);

          if(BYTES_PER_INSTRUCTION > BYTES_PER_WORD) {
            // Need to split the instruction in two.
            uint64_t val = (uint64_t)inst.toLong();
            Word first(val >> 32);
            Word second(val & 0xFFFFFFFF);
            data->push_back(first);
            data->push_back(second);
          }
          else data->push_back(inst);

          if(Debugger::mode == Debugger::DEBUGGER) printInstruction(inst, physPosition+i);
        }
      }
      else {
        // Data words are only 4 bytes long (I think).
        for(int i=0; i<size; i+=BYTES_PER_WORD) {
          Word w = nextWord(elfFile, false);
          data->push_back(w);
        }
      }

      // Put these instructions into a particular position in memory.
      DataBlock block(data, componentID_, physPosition);
      blocks->push_back(block);
    }

    delete &words;
  }

  fclose(terminalOutput);
  elfFile.close();

  // Find the position of main(), so we can get a core to fetch it.
  mainPos = findMain();

  return *blocks;
}

void ELFFileReader::addInstToVector(vector<Word>* vec, Instruction inst) {
  if(BYTES_PER_INSTRUCTION > BYTES_PER_WORD) {
    vec->push_back(inst.firstWord());
    vec->push_back(inst.secondWord());
  }
  else vec->push_back(inst);
}

Word ELFFileReader::nextWord(std::ifstream& file, bool isInstruction) const {
    uint64_t result=0, byte=0;

  if(isInstruction) {
    // Find a neater way to reverse endianness?
    byte = file.get();
    result |= byte << 32;
    byte = file.get();
    result |= byte << 40;
    byte = file.get();
    result |= byte << 48;
    byte = file.get();
    result |= byte << 56;

    byte = file.get();
    result |= byte << 0;
    byte = file.get();
    result |= byte << 8;
    byte = file.get();
    result |= byte << 16;
    byte = file.get();
    result |= byte << 24;

    return Instruction(result);
  }
  else {
    byte = file.get();
    result |= byte << 0;
    byte = file.get();
    result |= byte << 8;
    byte = file.get();
    result |= byte << 16;
    byte = file.get();
    result |= byte << 24;

    return Word(result);
  }
}

/* Generate a short program which allows a core to load the program in this
 * ELF file. The program involves setting a channel map entry and then fetching
 * from it. */
DataBlock& ELFFileReader::loaderProgram(const ComponentID& core, int mainPos) {
  if(Debugger::mode == Debugger::DEBUGGER)
    std::cout << "\nmain() is at address " << mainPos << std::endl;

  /*
  Instruction storeChannel("ori r3 r0 0");
  ChannelID firstMemInput(componentID_, 0);
  storeChannel.immediate(firstMemInput.getData());

  Instruction setfetchch("setchmap 0 r3");

  Instruction connect("ori r0 r0 0 > 0");
  ChannelID ipkCache = Cluster::IPKCacheInput(core_);
  connect.immediate(ipkCache.getData());

  Instruction nop("or r0 r0 r0");

  Instruction fetch("fetch.eop r0 0");
  fetch.immediate(mainPos);

  vector<Word>* instructions = new vector<Word>();
  addInstToVector(instructions, storeChannel);
  addInstToVector(instructions, nop);
  addInstToVector(instructions, setfetchch);
  addInstToVector(instructions, connect);
  addInstToVector(instructions, nop);
  addInstToVector(instructions, fetch);
  */

  Instruction inst01("ori                r3, r0, (0,8,0,3)         # First memory bank, first channel, quad bank group");

  Instruction inst02("or                 r0, r0, r0                # Make sure there are no timing problems");
  Instruction inst03("or                 r0, r0, r0");
  Instruction inst04("or                 r0, r0, r0");
  Instruction inst05("or                 r0, r0, r0");

  Instruction inst06("setchmap           0, r3                     # Set fetch channel to memory bank");

  Instruction inst07("ori                r4, r0, 0x00000103 > 0    # Configuration command: General purpose cache, eight banks");
  Instruction inst08("ori                r4, r0, 0x01000000 > 0    # Table update command ");
  Instruction inst09("ori                r4, r0, (0,0,1) > 0       # IPK cache of first core");

  Instruction inst10("or                 r0, r0, r0                # Wait for memory configuration to complete");
  Instruction inst11("or                 r0, r0, r0");
  Instruction inst12("or                 r0, r0, r0");
  Instruction inst13("or                 r0, r0, r0");
  Instruction inst14("or                 r0, r0, r0");
  Instruction inst15("or                 r0, r0, r0");
  Instruction inst16("or                 r0, r0, r0");
  Instruction inst17("or                 r0, r0, r0");

  Instruction inst18("ori                r5, r0, (0,8,1,3)         # Fifth memory bank, first channel, quad bank group");
  Instruction inst19("setchmap           1, r5                     # Set fetch channel to memory bank");

  Instruction inst20("or                 r0, r0, r0                # Make sure there are no timing problems");
  Instruction inst21("or                 r0, r0, r0");
  Instruction inst22("or                 r0, r0, r0");
  Instruction inst23("or                 r0, r0, r0");

  Instruction inst24("ori                r4, r0, 0x01000000 > 1    # Table update command ");
  Instruction inst25("ori                r4, r0, (0,0,2) > 1       # First data channel of first core");
  Instruction inst26("ori                r0, ch0, r0               # Wait for memory configuration to complete");

  Instruction inst27("fetch.eop          r0, 0                     # Fetch first instruction packet of program");
  inst27.immediate(mainPos);

  vector<Word>* instructions = new vector<Word>();
  addInstToVector(instructions, inst01);
  addInstToVector(instructions, inst02);
  addInstToVector(instructions, inst03);
  addInstToVector(instructions, inst04);
  addInstToVector(instructions, inst05);
  addInstToVector(instructions, inst06);
  addInstToVector(instructions, inst07);
  addInstToVector(instructions, inst08);
  addInstToVector(instructions, inst09);
  addInstToVector(instructions, inst10);
  addInstToVector(instructions, inst11);
  addInstToVector(instructions, inst12);
  addInstToVector(instructions, inst13);
  addInstToVector(instructions, inst14);
  addInstToVector(instructions, inst15);
  addInstToVector(instructions, inst16);
  addInstToVector(instructions, inst17);
  addInstToVector(instructions, inst18);
  addInstToVector(instructions, inst19);
  addInstToVector(instructions, inst20);
  addInstToVector(instructions, inst21);
  addInstToVector(instructions, inst22);
  addInstToVector(instructions, inst23);
  addInstToVector(instructions, inst24);
  addInstToVector(instructions, inst25);
  addInstToVector(instructions, inst26);
  addInstToVector(instructions, inst27);

//  cout << "Should give \"" << fetch << "\" to core " << core_ << endl;

  DataBlock* block = new DataBlock(instructions, core, 0);

  return *block;
}

int ELFFileReader::findMain() const {

  // Now looking for _start, not main, and it is always at 0x1000.
//  return 0x1000;

  string extFileName = filename_ + ".objdump-t";
  bool useExternalMetadata = access(extFileName.c_str(), F_OK) == 0;

  // Execute a command which returns information on all of the ELF sections.
  FILE* terminalOutput;

  if (useExternalMetadata) {
	  terminalOutput = fopen(extFileName.c_str(), "r");
  } else {
	  string command("loki-elf-objdump -t " + filename_);
	  terminalOutput = popen(command.c_str(), "r");
  }

  char line[100];
  int mainPos = 0;
  bool foundMainPos = false;

  // Step through each line of information, looking for the one corresponding
  // to main().
  while(fgets(line, 100, terminalOutput) != NULL) {
    string lineStr(line);
    vector<string>& words = StringManipulation::split(lineStr, ' ');

    // We're only interested one line of the information.
    if(words.back()=="_start\n") {
      mainPos = StringManipulation::strToInt("0x"+words[0]);
      foundMainPos = true;
      break;
    }

    delete &words;
  }

  fclose(terminalOutput);

  if(foundMainPos) {
    return mainPos;
  }
  else {
    std::cerr << "Error: unable to find main() in " << filename_ << std::endl;
    throw std::exception();
  }

}

ELFFileReader::ELFFileReader(const std::string& filename, const ComponentID& memory,
                             const ComponentID& core, const MemoryAddr location) :
    FileReader(filename, memory, location) {

  core_ = core;

}
