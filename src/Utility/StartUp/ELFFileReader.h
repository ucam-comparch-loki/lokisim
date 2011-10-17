/*
 * ELFFileReader.h
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef ELFFILEREADER_H_
#define ELFFILEREADER_H_

#include "FileReader.h"

class Word;

class ELFFileReader: public FileReader {

public:

  // Finds all useful data within the file, and returns all information needed
  // to put the data in the required components.
  virtual vector<DataBlock>& extractData(int& mainPos) const;

  // The ELF reader is different to the others, as it stores the contents of
  // the file in a memory, and then generates a small loader program for a
  // particular core to execute.
  ELFFileReader(const std::string& filename, const ComponentID& memory,
                const ComponentID& core, const MemoryAddr location);

private:

  // Get the next word from the file.
  Word nextWord(std::ifstream& file) const;

  // Find the position in memory where the start of the main() function will be.
  int findMain() const;

  // The core which will execute this program needs to be given a small loader
  // program.
  ComponentID core_;

};

#endif /* ELFFILEREADER_H_ */
