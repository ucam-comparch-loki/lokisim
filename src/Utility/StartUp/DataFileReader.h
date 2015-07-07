/*
 * DataFileReader.h
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef DATAFILEREADER_H_
#define DATAFILEREADER_H_

#include "FileReader.h"
#include "../../Datatype/Identifier.h"

class Word;

class DataFileReader: public FileReader {

public:

  // Finds all useful data within the file, and returns all information needed
  // to put the data in the required components.
  virtual vector<DataBlock>& extractData(int& mainPos);

  DataFileReader(std::string& filename, const ComponentID& component, MemoryAddr position);
  virtual ~DataFileReader();

private:

  Word nextWord(std::ifstream& file) const;

};

#endif /* DATAFILEREADER_H_ */
