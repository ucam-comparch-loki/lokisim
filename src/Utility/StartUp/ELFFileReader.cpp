/*
 * ELFFileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "ELFFileReader.h"
#include "DataBlock.h"
#include "../Debugger.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"
#include "../../TileComponents/Cluster.h"

vector<DataBlock>& ELFFileReader::extractData() const {
  vector<DataBlock>* blocks = new vector<DataBlock>();

  // Open the input file.
  std::ifstream elfFile(filename_.c_str());

  // Execute a command which returns information on all of the ELF sections.
  FILE* terminalOutput;
  string command("loki-elf-objdump -h " + filename_);
  terminalOutput = popen(command.c_str(), "r");

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

  blocks->push_back(loaderProgram());

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
DataBlock& ELFFileReader::loaderProgram() const {
  // Find the position of main(), so we can get a core to fetch it.
  int mainPos = findMain();
  if(Debugger::mode == Debugger::DEBUGGER)
    std::cout << "\nmain() is at address " << mainPos << std::endl;

  Instruction storeChannel("ori r3 r0 0");
  ChannelID firstMemInput = TileComponent::inputChannelID(componentID_, 0);
  storeChannel.immediate(firstMemInput);

  Instruction setfetchch("setchmap 0 r3");

  Instruction connect("ori r0 r0 0 > 0");
  ChannelID ipkCache = Cluster::IPKCacheInput(core_);
  connect.immediate(ipkCache);

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

//  cout << "Should give \"" << fetch << "\" to core " << core_ << endl;

  DataBlock* block = new DataBlock(instructions, core_);

  return *block;
}

int ELFFileReader::findMain() const {

  // Now looking for _start, not main, and it is always at 0x1000.
//  return 0x1000;

  // Execute a command which returns information on all of the ELF sections.
  FILE* terminalOutput;
  string command("loki-elf-objdump -t " + filename_);
  terminalOutput = popen(command.c_str(), "r");

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

ELFFileReader::ELFFileReader(std::string& filename, int memory, int component) :
    FileReader(filename, memory) {

  core_ = component;

}

ELFFileReader::~ELFFileReader() {

}
