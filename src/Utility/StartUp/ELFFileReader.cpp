/*
 * ELFFileReader.cpp
 *
 * Parsing of ELF binaries:
 *   http://wiki.osdev.org/ELF_Tutorial
 *   https://code.google.com/p/elfinfo/source/browse/trunk/elfinfo.c
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include <stdio.h>

#include "ELFFileReader.h"
#include "../Debugger.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Identifier.h"
#include "../../Datatype/Instruction.h"
#include "../../TileComponents/Core.h"

vector<DataBlock>& ELFFileReader::extractData(int& mainPos) {
  std::ifstream file(filename_.c_str());

  Elf32_Ehdr fileHeader;
  Elf32_Shdr sectionHeader;

  file.seekg(0, file.beg);
  readFileHeader(file, fileHeader);

  int numHeaders = fileHeader.e_shnum;
  for (int i=0; i<numHeaders; i++) {
    readSectionHeader(file, i, sectionHeader);
    processSection(file, sectionHeader);
  }

  file.close();

  return dataToLoad;
}

void ELFFileReader::readFileHeader(std::ifstream& file, Elf32_Ehdr& fileHeader) const {
  file.seekg(0, file.beg);
  file.read((char*)&fileHeader, sizeof(Elf32_Ehdr));
}

void ELFFileReader::readSectionHeader(std::ifstream& file, int sectionNumber, Elf32_Shdr& sectionHeader) const {
  Elf32_Ehdr fileHeader;
  readFileHeader(file, fileHeader);
  int numSections = fileHeader.e_shnum;

  assert(sectionNumber <= numSections);
  assert(sectionNumber >= 0);

  uint offset = fileHeader.e_shoff + fileHeader.e_shentsize*sectionNumber;
  file.seekg(offset, file.beg);
  file.read((char*)&sectionHeader, sizeof(Elf32_Shdr));
}

void ELFFileReader::processSection(std::ifstream& file, Elf32_Shdr& sectionHeader) {
  // We are only interested in sections to be loaded into memory.
  if ((sectionHeader.sh_flags & SHF_ALLOC) &&   // Alloc = put in memory
      (sectionHeader.sh_type != SHT_NOBITS)) {  // No bits = data not in ELF

    int size = sectionHeader.sh_size;
    int position = sectionHeader.sh_addr;
    int offset = sectionHeader.sh_offset;

    // Seek to the start of the segment.
    file.seekg(offset, file.beg);

    vector<Word>* data = new vector<Word>();

    if (sectionHeader.sh_flags & SHF_EXECINSTR) { // Executable section
      for (int i=0; i<size; i+=BYTES_PER_WORD) {
        Instruction inst = nextWord(file);
        data->push_back(inst);

        if (Debugger::mode == Debugger::DEBUGGER)
          printInstruction(inst, position+i);
      }
    }
    else {
      for (int i=0; i<size; i+=BYTES_PER_WORD) {
        Word w = nextWord(file);
        data->push_back(w);
      }
    }

    bool readOnly = !(sectionHeader.sh_flags & SHF_WRITE);

    // Put these instructions into a particular position in memory.
    dataToLoad.push_back(DataBlock(data, componentID_, position, readOnly));
  }
}

Word ELFFileReader::nextWord(std::ifstream& file) const {
  uint32_t result;
  file.read((char*)&result, 4);
  return Word(result);
}

ELFFileReader::ELFFileReader(const std::string& filename, const ComponentID& memory,
                             const ComponentID& core, const MemoryAddr location) :
    FileReader(filename, memory, location),
    core_(core) {

  // Do nothing

}
