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
#include "../TileComponents/WrappedTileComponent.h"
#include "../Datatype/Word.h"

using std::string;
using std::vector;

class CodeLoader {

public:

  // Read a file which tells which files to read.
  static void loadCode(string& settings, Tile& tile);

  // Store the contents of the given file into the component of the tile at
  // the given position.
  static void loadCode(string& filename, Tile& tile, int position);

  // Store the contents of the file into the given component.
  static void loadCode(string& filename, WrappedTileComponent& component);

  // Store the contents of the file into the given component.
  static void loadCode(string& filename, TileComponent& component);

  // Load instructions and data into all components of a tile.
  static void loadCode(Tile& tile, string& directory, vector<string>& coreFiles,
                                                      vector<string>& memFiles);

  static vector<Word>& getData(string& filename);
private:
  static bool isInstructionFile(string& filename);
  static Word makeWord(const string& str, bool instructionFile);

};

#endif /* CODELOADER_H_ */
