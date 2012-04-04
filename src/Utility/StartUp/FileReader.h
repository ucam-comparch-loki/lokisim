/*
 * FileReader.h
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef FILEREADER_H_
#define FILEREADER_H_

#include <string>
#include <vector>
#include "../../Typedefs.h"
#include "../../Datatype/ComponentID.h"

using std::string;
using std::vector;

class DataBlock;
class Instruction;

class FileReader {

public:

  // Finds all useful data within the file, and returns all information needed
  // to put the data in the required components.
  virtual vector<DataBlock>& extractData(int& mainPos) const = 0;

  // Parse the command (e.g. "12 code.loki") and generate a FileReader capable
  // of reading the specified filetype.
  // This method returns NULL if, for example, it is collecting a group of
  // object files to link together at the end.
  static FileReader* makeFileReader(const vector<string>& commandWords, bool customAppLoader);

  // If there are any object files which need linking together to form a single
  // executable, link them now.
  // Returns NULL if there are no files to link.
  static FileReader* linkFiles();

  // Returns whether a file with the given name exists.
  static bool exists(const string& filename);

  // Delete any temporary files created in the linking process.
  static void tidy();

  FileReader(const string& filename, const ComponentID& component, const MemoryAddr position=0);

protected:

  // Prints an instruction in a formatted way for the debugger. Address is
  // in bytes.
  static void printInstruction(Instruction i, MemoryAddr address);

  // Perform some small parameter-dependent transformations on the assembly
  // code. For example, change "ch1" to "r17".
  static void translateAssembly(string& infile, string& outfile);

  const string      filename_;
  const ComponentID componentID_;
  const MemoryAddr  position_;

private:

  static void deleteFile(string& filename);

  static vector<string> filesToLink;
  static vector<string> tempFiles;
  static string         linkedFile;

  // The linker always need to be invoked if an assembly file is involved. This
  // is because the linker puts the code in the correct position in memory.
  static bool           foundAsmFile;

};

#endif /* FILEREADER_H_ */
