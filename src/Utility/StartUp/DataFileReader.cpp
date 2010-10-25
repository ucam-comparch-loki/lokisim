/*
 * DataFileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "DataFileReader.h"
#include "DataBlock.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Data.h"

vector<DataBlock>& DataFileReader::extractData() const {
  std::ifstream file(filename_.c_str());
  vector<Word>* words = new vector<Word>();

  int wordsRead = 0;

  while(!file.fail()) {

    try {
      Word w = nextWord(file);
      words->push_back(w);
    }
    catch (std::exception& e) {
      continue; // If we couldn't make a valid word, try the next line
    }

    wordsRead++;

    if(file.eof()) break;

  }

  if(words->size() == 0)
    std::cerr << "Error: read 0 words from file " << filename_ << std::endl;
  else if(DEBUG) std::cout << "Retrieved " << words->size() <<
    " words from file " << filename_ << std::endl;

  file.close();

  vector<DataBlock>* blocks = new vector<DataBlock>();
  blocks->push_back(DataBlock(words, componentID_));

  return *blocks;
}

Word DataFileReader::nextWord(std::ifstream& file) const {
  // An array to store individual lines of the file in.
  char line[20];

  file.getline(line, 20, '\n');
  int val = StringManipulation::strToInt(line);
  return Data(val);
}

DataFileReader::DataFileReader(std::string& filename, int component) :
    FileReader(filename, component) {

}

DataFileReader::~DataFileReader() {

}
