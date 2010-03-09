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

class CodeLoader {

public:
  static void loadCode(char* filename, Tile& tile, int position);
  static void loadCode(char* filename, WrappedTileComponent& component);
  static void loadCode(char* filename, TileComponent& component);

private:
  static std::vector<Word>& getData(char* filename);

};

#endif /* CODELOADER_H_ */
