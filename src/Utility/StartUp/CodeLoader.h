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

#include "../../Chip.h"

using std::string;
using std::vector;

class DataBlock;
class FileReader;
class Instruction;

class CodeLoader {

private:

  static bool appLoaderInitialized;
  static int mainOffset;

public:

  // Read a file which tells which files to read.
  static void loadParameters(const string& settingsFile, chip_parameters_t& params);

  // Read a file which tells which files to read.
  static void loadCode(const string& settingsFile, Chip& chip);

  // If appropriate, link all object files together to make a single executable,
  // and load this program into the system.
  static void makeExecutable(Chip& chip);

private:

  // Store the contents of the given file into the component of the chip at
  // the given position. command is a vector of words from a loader file, e.g.:
  //   "12", "filename.loki"
  static void loadFromCommand(const vector<string>& command, Chip& chip, bool customAppLoader);

  static void loadFromReader(FileReader* reader, Chip& chip);

};

#endif /* CODELOADER_H_ */
