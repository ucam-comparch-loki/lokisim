/*
 * LokiFileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "LokiFileReader.h"
#include "CodeLoader.h"
#include "DataBlock.h"
#include "../../Datatype/Instruction.h"

vector<DataBlock>& LokiFileReader::extractData() const {
  std::ifstream file(filename_.c_str());
  vector<Word>* words = new vector<Word>();

  int wordsRead = 0;

  while(!file.fail()) {

    try {
      Word w = nextWord(file);
      words->push_back(w);

      if(CodeLoader::usingDebugger) {
        printInstruction((Instruction)w, wordsRead*BYTES_PER_WORD);
      }
    }
    catch (std::exception& e) {
      continue; // If we couldn't make a valid word, try the next line
    }

    wordsRead++;

    if(file.eof()) break;

  }

  if(words->size() == 0)
    std::cerr << "Error: read 0 words from file " << filename_ << endl;
  else if(DEBUG) cout << "Retrieved " << words->size() <<
    " words from file " << filename_ << endl;

  file.close();

  vector<DataBlock>* blocks = new vector<DataBlock>();
  blocks->push_back(DataBlock(words, componentID_));

  return *blocks;
}

Word LokiFileReader::nextWord(std::ifstream& file) const {
  // An array to store individual lines of the file in.
  char line[200];

  file.getline(line, 200, '\n');
  return Instruction(line);
}

LokiFileReader::LokiFileReader(std::string& filename, int component) :
    FileReader(filename, component) {

}

LokiFileReader::~LokiFileReader() {

}