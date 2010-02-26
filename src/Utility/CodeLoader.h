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

#include "../TileComponents/Cluster.h"
#include "../Datatype/Instruction.h"

class CodeLoader {

public:
  static void loadCode(char* filename, Cluster& cluster);

};

#endif /* CODELOADER_H_ */
