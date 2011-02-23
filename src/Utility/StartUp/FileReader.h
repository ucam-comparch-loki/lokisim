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
  static FileReader& makeFileReader(vector<std::string>& commandWords);

  FileReader(std::string& filename, ComponentID component, MemoryAddr position=0);
  virtual ~FileReader();

protected:

  // Prints an instruction in a formatted way for the debugger. Address is
  // in bytes.
  static void printInstruction(Instruction i, MemoryAddr address);

  // Perform some small parameter-dependent transformations on the assembly
  // code. For example, change "ch1" to "r17".
  static void translateAssembly(std::string& infile, std::string& outfile);

  std::string filename_;
  ComponentID componentID_;
  MemoryAddr position_;

};

#endif /* FILEREADER_H_ */
