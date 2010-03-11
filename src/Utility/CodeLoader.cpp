/*
 * CodeLoader.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "CodeLoader.h"
#include "../Datatype/Instruction.h"

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

/* Return a vector of Words corresponding to the contents of the file. */
std::vector<Word>& CodeLoader::getData(string& filename) {

  string fullName = "test_files/";
  fullName = fullName.append(filename);

  std::ifstream file(fullName.c_str());
  std::vector<Word> *instructions = new std::vector<Word>();

  // TODO: determine whether the file contains instructions or data

  char instruction[100];   // Is this enough?

  while(!file.fail()) {
    try {
      file.getline(instruction, 100, '\n');
      if(file.eof()) break;
      Instruction i(instruction);
      instructions->push_back(i);
    }
    catch(std::exception) {
      std::cerr << "Error: could not read file " << fullName << endl;
      break;
    }
  }

  if(instructions->size() == 0)
    std::cerr << "Error: read 0 instructions from file " << fullName << endl;

  file.close();

  return *instructions;

}
