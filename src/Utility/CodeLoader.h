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

#include "../Tile.h"

using std::string;
using std::vector;

class DataBlock;
class Instruction;
class Tile;
class TileComponent;
class WrappedTileComponent;

class CodeLoader {

public:

  static bool usingDebugger;

  // Read a file which tells which files to read.
  static void loadCode(string& settingsFile, Tile& tile);

  // Store the contents of the given file into the component of the tile at
  // the given position.
  static void loadCode(string& filename, Tile& tile, uint position);

private:
  static vector<DataBlock>& getData(string& filename);
  static uint8_t fileType(string& filename);
  static Word getWord(std::ifstream& file, uint8_t type);

  // Prints loaded instructions in a formatted way for the debugger. Address is
  // in bytes.
  static void printInstruction(Instruction i, int address);

  static vector<DataBlock>& readELFFile(string& filename);

  enum FILETYPE {LOKI, BINARY, DATA, ELF};

};

#endif /* CODELOADER_H_ */
