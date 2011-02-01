/*
 * DataFileReader.h
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef DATAFILEREADER_H_
#define DATAFILEREADER_H_

#include "FileReader.h"

class Word;

class DataFileReader: public FileReader {

public:

  // Finds all useful data within the file, and returns all information needed
  // to put the data in the required components.
  virtual vector<DataBlock>& extractData() const;

  DataFileReader(std::string& filename, ComponentID component, MemoryAddr position);
  virtual ~DataFileReader();

private:

  Word nextWord(std::ifstream& file) const;

};

#endif /* DATAFILEREADER_H_ */
