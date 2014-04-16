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
#include "../../Datatype/ComponentID.h"
#include "../../Datatype/Word.h"

vector<DataBlock>& DataFileReader::extractData(int& mainPos) const {
  std::ifstream file(filename_.c_str());
  vector<Word>* words = new vector<Word>();

  int wordsRead = 0;

  while (!file.fail()) {

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

  if (words->size() == 0)
    std::cerr << "Error: read 0 words from file " << filename_ << std::endl;
  else if(DEBUG) std::cout << "Retrieved " << words->size() <<
    " words from file " << filename_ << std::endl;

  file.close();

  vector<DataBlock>* blocks = new vector<DataBlock>();
  blocks->push_back(DataBlock(words, componentID_, position_, false));

  return *blocks;
}

Word DataFileReader::nextWord(std::ifstream& file) const {
  // An array to store individual lines of the file in.
  char line[20];

  file.getline(line, 20, '\n');

  // Filter out any lines which contain nothing, or are comments.
  if (line[0] == '\0' || line[0] == ';' || line[0] == '%')
    throw std::exception();
  else {
    // If there are multiple words on a line, we're only interested in the
    // first one. The rest are comments.
    string str(line);

    vector<string>& words = StringManipulation::split(str, ' ');
    string first = words[0];
    delete &words;

    int val = StringManipulation::strToInt(first);
    return Word(val);
  }
}

DataFileReader::DataFileReader(std::string& filename, const ComponentID& component, MemoryAddr position) :
    FileReader(filename, component, position) {

}

DataFileReader::~DataFileReader() {

}
