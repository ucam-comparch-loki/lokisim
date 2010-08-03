/*
 * CodeLoader.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "CodeLoader.h"
#include "Parameters.h"
#include "StringManipulation.h"
#include "../Datatype/Instruction.h"

/* Use an external file to tell which files to read.
 * This file should optionally start with a "directory ..." line, and should
 * then contain any number of lines of the form:
 *   component_id filename
 * where "filename" doesn't contain any spaces. */
void CodeLoader::loadCode(string& settings, Tile& tile) {

}

/* Load code from the specified file into a particular component of the
 * given tile. */
void CodeLoader::loadCode(string& filename, Tile& tile, int position) {
  tile.storeData(getData(filename), position);
}

/* Load code from the specified file into the given component. */
void CodeLoader::loadCode(string& filename, WrappedTileComponent& component) {
  component.storeData(getData(filename));
}

/* Load code from the specified file into the given component. */
void CodeLoader::loadCode(string& filename, TileComponent& component) {
  component.storeData(getData(filename));
}

/* Load instructions and data into all components of a tile. */
void CodeLoader::loadCode(Tile& tile, string& directory,
                          vector<string>& coreFiles, vector<string>& memFiles) {

  if(DEBUG) std::cout << "Initialising..." << endl;

  for(unsigned int i=0; i<coreFiles.size(); i++) {
    string filename = directory + "/" + coreFiles[i];
    loadCode(filename, tile, i);
  }

  for(unsigned int i=0; i<memFiles.size(); i++) {
    string filename = directory + "/" + memFiles[i];
    loadCode(filename, tile, CLUSTERS_PER_TILE + i);
  }

  if(DEBUG) std::cout << endl;

}

/* Return a vector of Words corresponding to the contents of the file. */
vector<Word>& CodeLoader::getData(string& filename) {

  string fullName = "test_files/";
  fullName = fullName.append(filename);

  std::ifstream file(fullName.c_str());
  vector<Word>* words = new vector<Word>();

  // See if this file contains Instructions or Data
  bool instructionFile = isInstructionFile(filename);

  // An array to load a line of the file into. Is 200 characters enough?
  char wordAsString[200];

  while(!file.fail()) {
    try {
      file.getline(wordAsString, 200, '\n');

      try {
        Word w = makeWord(wordAsString, instructionFile);
        words->push_back(w);
      }
      catch (std::exception e) {
        continue; // If we couldn't make a valid word, try the next line
      }

      if(file.eof()) break;

    }
    catch(std::exception) {
      std::cerr << "Error: could not read file " << fullName << endl;
      break;
    }
  }

  if(words->size() == 0)
    std::cerr << "Error: read 0 words from file " << fullName << endl;
  else if(DEBUG) std::cout << "Retrieved " << words->size() <<
    " words from file " << fullName << endl;

  file.close();

  return *words;

}

/* Returns whether the file should contain instructions. If not, it should
 * contain data. Instruction files are of type .loki, and data files are
 * of type .data. */
bool CodeLoader::isInstructionFile(string& filename) {

  vector<string> parts = StringManipulation::split(filename, '.');

  if(parts.size() < 2) {
    std::cerr << "Error: no file extension for file " << filename << endl;
    throw std::exception();
  }

  if(parts[1] == "loki") return true;
  else if(parts[1] == "data") return false;
  else std::cerr << "Unknown file format: " << filename << endl;

  return false;

}

/* Return either an Instruction or a Data represented by the given string,
 * depending on the type of file being read. */
Word CodeLoader::makeWord(const string& str, bool instructionFile) {
  if(instructionFile) {
    return Instruction(str);
  }
  else {
    int val = StringManipulation::strToInt(str);
    return Data(val);
  }
}
