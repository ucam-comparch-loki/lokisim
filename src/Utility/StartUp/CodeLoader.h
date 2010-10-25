/*
 * CodeLoader.h
 *
 * Class with useful methods for loading code from external files into memory
 * before execution begins.
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#ifndef CODELOADER_H_
#define CODELOADER_H_

#include "../../Tile.h"

using std::string;
using std::vector;

class DataBlock;
class Instruction;
class Tile;

class CodeLoader {

public:

  static bool usingDebugger;

  // Read a file which tells which files to read.
  static void loadCode(string& settingsFile, Tile& tile);

private:

  // Store the contents of the given file into the component of the tile at
  // the given position. command is a vector of words from a loader file, e.g.:
  //   "12", "filename.loki"
  static void loadCode(vector<string>& command, Tile& tile);

};

#endif /* CODELOADER_H_ */
