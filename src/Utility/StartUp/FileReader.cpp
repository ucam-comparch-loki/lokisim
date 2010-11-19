/*
 * FileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "FileReader.h"
#include "DataFileReader.h"
#include "ELFFileReader.h"
#include "LokiFileReader.h"
#include "LokiBinaryFileReader.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

/* Print an instruction in the form:
 *   [position] instruction
 * in such a way that everything aligns nicely. */
void FileReader::printInstruction(Instruction i, int address) {
  // Set up some output formatting so numbers are all the same length.
  std::cout.fill('0');

  std::cout << "\[";
  std::cout.width(4);
  std::cout << std::right << address << "]\t" << i << std::endl;

  // Undo the formatting changes.
  std::cout.fill(' ');
}

FileReader& FileReader::makeFileReader(vector<std::string>& words) {
  FileReader* reader;

  if(words.size()<2 || words.size()>3) {
    std::cerr << "Error: wrong number of loader file arguments." << std::endl;
    throw std::exception();
  }
  else if(words.size()==2) {
    int component = StringManipulation::strToInt(words[0]);

    // Split the filename into the name and the extension.
    vector<std::string>& parts = StringManipulation::split(words[1], '.');

    if(parts.back() == "loki") {
      reader = new LokiFileReader(words[1], component);
    }
    else if(parts.back() == "data") {
      reader = new DataFileReader(words[1], component);
    }
    else if(parts.back() == "bloki") {
      reader = new LokiBinaryFileReader(words[1], component);
    }
    else std::cerr << "Unknown file format: " << words[1] << std::endl;

  }
  else if(words.size()==3) {
    int memory = StringManipulation::strToInt(words[0]);
    int core = StringManipulation::strToInt(words[1]);

    // TODO: change BYTES_PER_WORD to 8, before the chip is made.

    reader = new ELFFileReader(words[2], memory, core);
  }

  return *reader;
}

FileReader::FileReader(std::string& filename, int component) {
  filename_ = filename;
  componentID_ = component;
}

FileReader::~FileReader() {

}
