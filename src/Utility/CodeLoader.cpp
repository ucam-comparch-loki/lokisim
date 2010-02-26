/*
 * CodeLoader.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "CodeLoader.h"

/* Load code from the specified file into the IPK cache of the given cluster.
 * Expand this to allow any TileComponent to have code loaded into it? */
void CodeLoader::loadCode(char* filename, Cluster& cluster) {

  std::ifstream file(filename);
  std::vector<Instruction> instructions;

  char instruction[100];   // Is this enough?

  while(!file.fail()) {
    try {
      file.getline(instruction, 100, '\n');
      if(file.eof()) break;
      Instruction i(instruction);
      instructions.push_back(i);
    }
    catch(std::exception) {
      break;
    }
  }

  file.close();

  cluster.storeCode(instructions);

}
