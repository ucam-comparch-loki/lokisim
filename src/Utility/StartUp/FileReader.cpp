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
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

/* Print an instruction in the form:
 *   [position] instruction
 * in such a way that everything aligns nicely. */
void FileReader::printInstruction(Instruction i, MemoryAddr address) {
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

  if(words.size() == 1) {
    // Assume that we want the code to go into the first memory (component
    // with ID CORES_PER_TILE) and that the first core should execute it.
    reader = new ELFFileReader(words[0], CORES_PER_TILE, 0);
  }
  else if(words.size()<2 || words.size()>3) {
    std::cerr << "Error: wrong number of loader file arguments." << std::endl;
    throw std::exception();
  }
  else /*if(words.size()==2)*/ {
    int component = StringManipulation::strToInt(words[0]);
    int position = 0;
    std::string filename;

    if(words.size()==3) {
      position = StringManipulation::strToInt(words[1]);
      filename = words[2];
    }
    else {
      filename = words[1];
    }

    // Split the filename into the name and the extension.
    vector<std::string>& parts = StringManipulation::split(filename, '.');

    if(parts.size() == 1) {
      reader = new ELFFileReader(filename, component, position);
    }
    else if(parts.back() == "loki") {
      reader = new LokiFileReader(filename, component, position);
    }
    else if(parts.back() == "data") {
      reader = new DataFileReader(filename, component, position);
    }
    else if(parts.back() == "bloki") {
      reader = new LokiBinaryFileReader(filename, component, position);
    }
    else {
      std::cerr << "Unknown file format: " << filename << std::endl;
      throw std::exception();
    }

  }

  return *reader;
}

FileReader::FileReader(std::string& filename, ComponentID component, MemoryAddr position) {
  filename_ = filename;
  componentID_ = component;
  position_ = position;
}

FileReader::~FileReader() {

}
