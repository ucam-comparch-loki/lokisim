/*
 * LokiFileReader.h
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef LOKIFILEREADER_H_
#define LOKIFILEREADER_H_

#include "FileReader.h"

class Word;

class LokiFileReader: public FileReader {

public:

  // Finds all useful data within the file, and returns all information needed
  // to put the data in the required components.
  virtual vector<DataBlock>& extractData(int& mainPos);

  LokiFileReader(std::string& filename, const ComponentID& component, MemoryAddr position);
  virtual ~LokiFileReader();

protected:

  virtual Word nextWord(std::ifstream& file) const;

};

#endif /* LOKIFILEREADER_H_ */
