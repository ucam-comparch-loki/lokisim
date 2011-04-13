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

using std::string;
using std::vector;

class DataBlock;
class Instruction;

class FileReader {

public:

  // Finds all useful data within the file, and returns all information needed
  // to put the data in the required components.
  virtual vector<DataBlock>& extractData() const = 0;

  // Parse the command (e.g. "12 code.loki") and generate a FileReader capable
  // of reading the specified filetype.
  // This method returns NULL if, for example, it is collecting a group of
  // object files to link together at the end.
  static FileReader* makeFileReader(vector<string>& commandWords);

  // If there are any object files which need linking together to form a single
  // executable, link them now.
  // Returns NULL if there are no files to link.
  static FileReader* linkFiles();

  // Delete any temporary files created in the linking process.
  static void tidy();

  FileReader(string& filename, ComponentID component, MemoryAddr position=0);

protected:

  // Prints an instruction in a formatted way for the debugger. Address is
  // in bytes.
  static void printInstruction(Instruction i, MemoryAddr address);

  // Perform some small parameter-dependent transformations on the assembly
  // code. For example, change "ch1" to "r17".
  static void translateAssembly(string& infile, string& outfile);

  string      filename_;
  ComponentID componentID_;
  MemoryAddr  position_;

private:

  static void deleteFile(string& filename);

  static vector<string> filesToLink;
  static string         linkedFile;

};

#endif /* FILEREADER_H_ */
