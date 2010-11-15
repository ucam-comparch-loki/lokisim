/*
 * ELFFileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "ELFFileReader.h"
#include "CodeLoader.h"
#include "DataBlock.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

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
  while(!feof(terminalOutput)) {
    fgets(line, 100, terminalOutput);
    string lineStr(line);
    vector<string>& words = StringManipulation::split(lineStr, ' ');

    // We're only interested in a few particular lines of the information.
    if(words.size() == 7 && (words[1]==".text" || words[1]==".data")) {
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
        for(int i=0; i<size; i+=8) {
          Instruction inst = nextWord(elfFile, true);
          data->push_back(inst);

          if(CodeLoader::usingDebugger) printInstruction(inst, physPosition+i);
        }
      }
      else {
        // Data words are only 4 bytes long (I think).
        for(int i=0; i<size; i+=4) {
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
  if(CodeLoader::usingDebugger) cout << "\nmain() is at address " << mainPos << endl;

  Instruction storeChannel("ori r3 r0 0");
  storeChannel.immediate(componentID_*NUM_CLUSTER_INPUTS); // Memory's first input.

  Instruction setfetchch("setchmap r3 0");

  Instruction connect("ori r0 r0 0 > 0");
  connect.immediate(core_*NUM_CLUSTER_INPUTS + 1); // Core's IPK cache

  Instruction nop("nop r0 r0 r0");

  Instruction fetch("fetch.eop r0 0");
  fetch.immediate(mainPos);

  vector<Word>* instructions = new vector<Word>();
  instructions->push_back(storeChannel);
  instructions->push_back(nop);
  instructions->push_back(nop); // Need 2 nops to ensure that r3 has been written.
  instructions->push_back(setfetchch);
  instructions->push_back(connect);
  instructions->push_back(nop);
  instructions->push_back(nop); // Make sure we have connected to mem before fetch.
  instructions->push_back(fetch);

  cout << "Should give \"" << fetch << "\" to core " << core_ << endl;

  DataBlock* block = new DataBlock(instructions, core_);

  return *block;
}

int ELFFileReader::findMain() const {

  // Execute a command which returns information on all of the ELF sections.
  FILE* terminalOutput;
  string command("loki-elf-objdump -t " + filename_);
  terminalOutput = popen(command.c_str(), "r");

  char line[100];
  int mainPos;
  bool foundMainPos = false;

  // Step through each line of information, looking for the one corresponding
  // to main().
  while(!feof(terminalOutput)) {
    fgets(line, 100, terminalOutput);
    string lineStr(line);
    vector<string>& words = StringManipulation::split(lineStr, ' ');

    // We're only interested one line of the information.
    if(words.back()=="main\n") {
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
    cerr << "Error: unable to find main() in " << filename_ << endl;
    throw std::exception();
  }

}

ELFFileReader::ELFFileReader(std::string& filename, int memory, int component) :
    FileReader(filename, memory) {

  core_ = component;

}

ELFFileReader::~ELFFileReader() {

}
