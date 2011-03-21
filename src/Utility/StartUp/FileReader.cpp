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
#include "../../Datatype/DecodedInst.h"
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
  std::string filename;
  int component;
  int position;

  if(words.size() == 1) {
    // Assume that we want the code to go into the first memory (component
    // with ID CORES_PER_TILE) and that the first core should execute it.
    filename = words[0];
    component = CORES_PER_TILE;
    position = 0;
  }
  else if(words.size() == 2) {
    component = StringManipulation::strToInt(words[0]);
    position = 0;
    filename = words[1];
  }
  else {
    std::cerr << "Error: wrong number of loader file arguments." << std::endl;
    throw std::exception();
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
  else if(parts.back() == "s") {
    string tempfile = parts.front() + "_temp.s";
    string elfFile = parts.front();
    translateAssembly(filename, tempfile);

    string makeELF = "loki-elf-as " + tempfile + " -o " + elfFile;
    string deleteTemp = "rm " + tempfile;

    int failure = system(makeELF.c_str());
    if(failure) {
      cerr << "Error: unable to use loki-elf-as" << endl;
      throw std::exception();
    }

    failure = system(deleteTemp.c_str());
    if(failure) cerr << "Warning: unable to delete temporary files." << endl;

    reader = new ELFFileReader(elfFile, component, position);
  }
  else {
    std::cerr << "Unknown file format: " << filename << std::endl;
    throw std::exception();
  }

  delete &parts;
  return *reader;
}

void FileReader::translateAssembly(std::string& infile, std::string& outfile) {
  std::ifstream in(infile.c_str());
  std::ofstream out(outfile.c_str());

  char line[200];   // An array of chars to load a line from the file into.

  while(!in.fail()) {
    in.getline(line, 200);

    try {
      // Try to make an instruction out of the line. In creating the
      // instruction, all of the transformations we want to do are performed.
      Instruction i(line);
      DecodedInst dec(i);
      std::stringstream ss;
      ss << dec;
      out << "\t" << ss.rdbuf()->str() << std::endl;
    }
    catch(std::exception& e) {
      // If we couldn't make an instruction, just copy the line across.
      // It may be a label or some other important line.
      out << line << std::endl;
    }
  }

  in.close();
  out.close();
}

FileReader::FileReader(std::string& filename, ComponentID component, MemoryAddr position) {
  filename_ = filename;
  componentID_ = component;
  position_ = position;
}

FileReader::~FileReader() {

}
