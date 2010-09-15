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
#include "../TileComponents/TileComponent.h"

bool CodeLoader::usingDebugger = false;

/* Use an external file to tell which files to read.
 * The file should contain lines of the following forms:
 *   directory directory_name
 *     Change the current directory
 *   loader file_name
 *     Use another file loader - useful for loading multiple (sub-)programs
 *   component_id file_name
 *     Load the contents of the file into the component */
void CodeLoader::loadCode(string& settings, Tile& tile) {

  char line[200];   // An array of chars to load a line from the file into.
  string fullName = "test_files/" + settings;
  std::ifstream file(fullName.c_str());
  string directory("");

  while(!file.fail()) {
    try {
      file.getline(line, 200, '\n');
      string s(line);

      if(s[0]=='%' || s[0]=='\0') continue;   // Skip past any comments

      vector<string> words = StringManipulation::split(s, ' ');

      if(words[0]=="directory") {     // Update the current directory
        directory = words[1];
      }
      else if(words[0]=="loader") {   // Use another file loader
        string loaderFile = directory + "/" + words[1];
        loadCode(loaderFile, tile);
      }
      else {                          // Load code/data from the given file
        int index = StringManipulation::strToInt(words[0]);
        string filename = directory + "/" + words[1];
        loadCode(filename, tile, index);
      }

      if(file.eof()) break;
    }
    catch(std::exception e) {
      std::cerr << "Error: could not read file " << settings << endl;
      break;
    }
  }

  file.close();

}

/* Load code from the specified file into a particular component of the
 * given tile. */
void CodeLoader::loadCode(string& filename, Tile& tile, int position) {
  if(usingDebugger) cout << "\nLoading into " <<
      (position>=CLUSTERS_PER_TILE ? "memory " : "core ") << position << ":\n";

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
  int typeOfFile = fileType(filename);

  int wordsRead = 0;

  while(!file.fail()) {
    try {

      try {
        Word w = getWord(file, typeOfFile);
        words->push_back(w);

        if(usingDebugger && (typeOfFile != DATA)) {
          cout << (wordsRead*BYTES_PER_WORD) << "\t"
               << static_cast<Instruction>(w) << endl;
        }
      }
      catch (std::exception e) {
        continue; // If we couldn't make a valid word, try the next line
      }

      if(file.eof()) break;

    }
    catch(std::exception e) {
      std::cerr << "Error: could not read file " << fullName << endl;
      break;
    }

    wordsRead++;
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
int CodeLoader::fileType(string& filename) {

  vector<string> parts = StringManipulation::split(filename, '.');

  if(parts.size() < 2) {
    std::cerr << "Error: no file extension for file " << filename << endl;
    throw std::exception();
  }

  if(parts[1] == "loki") return LOKI;
  else if(parts[1] == "data") return DATA;
  else if(parts[1] == "bloki") return BINARY;
  else std::cerr << "Unknown file format: " << filename << endl;

  return DATA;

}

/* Return either an Instruction or a Data from the file, depending on the
 * type of file being read. */
Word CodeLoader::getWord(std::ifstream& file, int type) {

  // An array to store individual lines of the file in.
  static char line[200];

  switch(type) {
    case LOKI: {
      file.getline(line, 200, '\n');
      return Instruction(line);
    }
    case BINARY: {
      // At the moment, binary files are formatted as 32-bit words in hex.
      // We therefore need to load in two words to make a single instruction.
      file.getline(line, 200, '\n');
      unsigned long val = (unsigned long)StringManipulation::strToInt(line) << 32;
      file.getline(line, 200, '\n');
      val += (unsigned long)StringManipulation::strToInt(line);
      return Instruction(val);
    }
    case DATA: {
      file.getline(line, 200, '\n');
      int val = StringManipulation::strToInt(line);
      return Data(val);
    }
    default: {
      std::cerr << "Error: unknown file format." << endl;
      throw std::exception();
    }
  }

}
