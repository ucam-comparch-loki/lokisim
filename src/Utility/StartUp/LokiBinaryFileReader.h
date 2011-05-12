/*
 * LokiBinaryFileReader.h
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef LOKIBINARYFILEREADER_H_
#define LOKIBINARYFILEREADER_H_

#include "LokiFileReader.h"

class LokiBinaryFileReader: public LokiFileReader {

public:

  LokiBinaryFileReader(std::string& filename, const ComponentID& component, MemoryAddr position);
  virtual ~LokiBinaryFileReader();

protected:

  virtual Word nextWord(std::ifstream& file) const;

};

#endif /* LOKIBINARYFILEREADER_H_ */
