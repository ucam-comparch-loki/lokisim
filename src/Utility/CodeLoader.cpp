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
void CodeLoader::loadCode(char* filename, Tile& tile, int position) {
  tile.storeData(getData(filename), position);
}

/* Load code from the specified file into the given component. */
void CodeLoader::loadCode(char* filename, WrappedTileComponent& component) {
  component.storeData(getData(filename));
}

/* Load code from the specified file into the given component. */
void CodeLoader::loadCode(char* filename, TileComponent& component) {
  component.storeData(getData(filename));
}

/* Return a vector of Words corresponding to the contents of the file. */
std::vector<Word>& CodeLoader::getData(char* filename) {

  std::ifstream file(filename);
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
      break;
    }
  }

  file.close();

  return *instructions;

}
