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

class Tile;
class TileComponent;
class Word;
class WrappedTileComponent;

class CodeLoader {

public:

  static bool usingDebugger;

  // Read a file which tells which files to read.
  static void loadCode(string& settings, Tile& tile);

  // Store the contents of the given file into the component of the tile at
  // the given position.
  static void loadCode(string& filename, Tile& tile, uint position);

  // Store the contents of the file into the given component.
  static void loadCode(string& filename, WrappedTileComponent& component);

  // Store the contents of the file into the given component.
  static void loadCode(string& filename, TileComponent& component);

  // Load instructions and data into all components of a tile.
  static void loadCode(Tile& tile, string& directory, vector<string>& coreFiles,
                                                      vector<string>& memFiles);

  static vector<Word>& getData(string& filename);
private:
  static uint8_t fileType(string& filename);
  static Word getWord(std::ifstream& file, uint8_t type);

  enum FILETYPE {LOKI, BINARY, DATA};

};

#endif /* CODELOADER_H_ */
