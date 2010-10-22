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

class CodeLoader {

public:

  static bool usingDebugger;

  // Read a file which tells which files to read.
  static void loadCode(string& settingsFile, Tile& tile);

  // Store the contents of the given file into the component of the tile at
  // the given position.
  static void loadCode(string& filename, Tile& tile, uint position);

private:

  // Return a list of all blocks of data in the given file, and the positions
  // they should be put in memory.
  static vector<DataBlock>& getData(string& filename);

  // Return the FileType of the file, determined using the file extension.
  static uint8_t fileType(string& filename);

  // Read the next word (data or instruction) from the given file.
  static Word getWord(std::ifstream& file, uint8_t type);

  // Prints an instruction in a formatted way for the debugger. Address is
  // in bytes.
  static void printInstruction(Instruction i, int address);

  // Read in all blocks of code/data from the file, along with the locations in
  // memory where they should be loaded.
  static vector<DataBlock>& readELFFile(string& filename);

  // Find the position in memory where the start of the main() function will be.
  // The given file must be in the ELF format.
  static int findMain(string& filename);

  enum FileType {LOKI, BINARY, DATA, ELF};

};

#endif /* CODELOADER_H_ */
