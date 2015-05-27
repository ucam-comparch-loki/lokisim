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
#include "../../Datatype/Identifier.h"
#include "../../Datatype/Instruction.h"
#include "../../TileComponents/Core.h"

vector<DataBlock>& ELFFileReader::extractData(int& mainPos) const {
  vector<DataBlock>* blocks = new vector<DataBlock>();

  // Open the input file.
  std::ifstream elfFile(filename_.c_str());

  // For machines which can't run the Loki toolchain, we provide the section
  // headers in a separate file.
  string extFileName = filename_ + ".objdump-h";
  bool useExternalMetadata = exists(extFileName);

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
  while (fgets(line, 100, terminalOutput) != NULL) {
    string lineStr(line);
    vector<string>& words = StringManipulation::split(lineStr, ' ');

    // We're only interested in descriptions of each section.
    if (words.size() == 7) {
      char properties[100];
      if (fgets(properties, 100, terminalOutput) == NULL)
        break;

      // We are only interested in sections with the "LOAD" property.
      if (strstr(properties, "LOAD") == NULL)
        continue;

      string name      = words[1];
      int size         = StringManipulation::strToInt("0x"+words[2]);
//      int virtPosition = StringManipulation::strToInt("0x"+words[3]);
      int physPosition = StringManipulation::strToInt("0x"+words[4]);
      int offset       = StringManipulation::strToInt("0x"+words[5]);

      // Seek to "offset" in elfFile.
      elfFile.seekg(offset, elfFile.beg);

      vector<Word>* data = new vector<Word>();

      if (name == ".text") {
        for (int i=0; i<size; i+=BYTES_PER_WORD) {
          Instruction inst = nextWord(elfFile);
          data->push_back(inst);

          if(Debugger::mode == Debugger::DEBUGGER) printInstruction(inst, physPosition+i);
        }
      }
      else {
        for (int i=0; i<size; i+=BYTES_PER_WORD) {
          Word w = nextWord(elfFile);
          data->push_back(w);
        }
      }

      bool readOnly = (strstr(properties, "READONLY") != NULL);

      // Put these instructions into a particular position in memory.
      DataBlock block(data, componentID_, physPosition, readOnly);
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

Word ELFFileReader::nextWord(std::ifstream& file) const {
  uint64_t result=0, byte=0;

  // Find a neater way to reverse endianness? ntohs?
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

int ELFFileReader::findMain() const {

  // Now looking for _start, not main, and it is always at 0x1000.
//  return 0x1000;

  string extFileName = filename_ + ".objdump-t";
  bool useExternalMetadata = exists(extFileName);

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
  while (fgets(line, 100, terminalOutput) != NULL) {
    string lineStr(line);
    vector<string>& words = StringManipulation::split(lineStr, ' ');

    // We're only interested one line of the information.
    if (words.back()=="_start\n") {
      mainPos = StringManipulation::strToInt("0x"+words[0]);
      foundMainPos = true;
      delete &words;
      break;
    }

    delete &words;
  }

  fclose(terminalOutput);

  if (foundMainPos) {
    return mainPos;
  }
  else {
    std::cerr << "Error: unable to find main() in " << filename_ << std::endl;
    throw std::exception();
  }

}

ELFFileReader::ELFFileReader(const std::string& filename, const ComponentID& memory,
                             const ComponentID& core, const MemoryAddr location) :
    FileReader(filename, memory, location),
    core_(core) {

  // Do nothing

}
