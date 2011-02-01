/*
 * LokiBinaryFileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "LokiBinaryFileReader.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

Word LokiBinaryFileReader::nextWord(std::ifstream& file) const {
  // An array to store individual lines of the file in.
  char line[20];

  // At the moment, binary files are formatted as 32-bit words in hex.
  // We therefore need to load in two words to make a single instruction.
  file.getline(line, 20, '\n');
  uint64_t val = (uint64_t)StringManipulation::strToInt(line) << 32;
  file.getline(line, 20, '\n');
  val += (uint64_t)StringManipulation::strToInt(line);
  return Instruction(val);
}

LokiBinaryFileReader::LokiBinaryFileReader(std::string& filename, ComponentID component, MemoryAddr position) :
    LokiFileReader(filename, component, position) {

}

LokiBinaryFileReader::~LokiBinaryFileReader() {

}
