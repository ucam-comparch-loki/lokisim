/*
 * CodeLoader.h
 *
 * Class with useful methods for loading code from external files into memory
 * before execution begins.
 *
 * Extend to BinaryCodeLoader and AssemblyCodeLoader?
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
  static void loadCode(string& filename, Tile& tile, int position);
  static void loadCode(string& filename, WrappedTileComponent& component);
  static void loadCode(string& filename, TileComponent& component);

private:
  static vector<Word>& getData(string& filename);
  static bool isInstructionFile(string& filename);
  static Word makeWord(const string& str, bool instructionFile);

};

#endif /* CODELOADER_H_ */
