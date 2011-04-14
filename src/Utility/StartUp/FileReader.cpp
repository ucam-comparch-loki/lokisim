/*
 * FileReader.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "FileReader.h"
#include "DataFileReader.h"
#include "ELFFileReader.h"
#include "LokiFileReader.h"
#include "LokiBinaryFileReader.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

#include <sstream>
#include <sys/stat.h>

using std::cout;
using std::cerr;
using std::endl;

vector<string> FileReader::filesToLink;
string         FileReader::linkedFile;

/* Print an instruction in the form:
 *   [position] instruction
 * in such a way that everything aligns nicely. */
void FileReader::printInstruction(Instruction i, MemoryAddr address) {
  // Set up some output formatting so numbers are all the same length.
  cout.fill('0');

  cout << "\[";
  cout.width(4);
  cout << std::right << address << "]\t" << i << endl;

  // Undo the formatting changes.
  cout.fill(' ');
}

FileReader* FileReader::makeFileReader(vector<string>& words) {
  FileReader* reader;
  string filename;
  int component;
  int position;

  if(words.size() == 1) {
    // Assume that we want the code to go into the first memory (component
    // with ID CORES_PER_TILE) and that the first core should execute it.
    filename = words[0];
    component = CORES_PER_TILE;
    position = 0;
  }
  else if(words.size() == 2) {
    component = StringManipulation::strToInt(words[0]);
    position = 0;
    filename = words[1];
  }
  else {
    cerr << "Error: wrong number of loader file arguments." << endl;
    tidy();
    throw std::exception();
  }

  // Split the filename into the name and the extension.
  int dotPos = filename.find_last_of('.');
  string name = filename.substr(0, dotPos);
  string extension = filename.substr(dotPos+1);

  if(extension == "") {
    filesToLink.push_back(name);
    reader = NULL;
  }
  else if(extension == "loki") {
    reader = new LokiFileReader(filename, component, position);
  }
  else if(extension == "data") {
    reader = new DataFileReader(filename, component, position);
  }
  else if(extension == "bloki") {
    reader = new LokiBinaryFileReader(filename, component, position);
  }
  else if(extension == "s") {
    string asmFile, elfFile;

    // Add a salt to temporary filenames so there aren't conflicts if multiple
    // instances of the simulator run at once.
    int salt = getpid();
    std::stringstream uniqueName;
    uniqueName << name << salt;
    uniqueName >> elfFile;
    asmFile = elfFile + ".s";

    translateAssembly(filename, asmFile);

    string makeELF = "loki-elf-as " + asmFile + " -o " + elfFile;

    int failure = system(makeELF.c_str());
    if(failure) {
      cerr << "Error: unable to assemble file using command:\n  " << makeELF << endl;
      tidy();
      throw std::exception();
    }

    deleteFile(asmFile);

    filesToLink.push_back(elfFile);
    reader = NULL;
  }
  else {
    cerr << "Unknown file format: " << filename << endl;
    tidy();
    throw std::exception();
  }

  return reader;
}

FileReader* FileReader::linkFiles() {
  switch(filesToLink.size()) {
    case 0: return NULL;
    case 1: return new ELFFileReader(filesToLink[0], CORES_PER_TILE, 0);
    default: {
      // Make sure the linking library exists.
      // TODO: make library location a parameter.
      // Could store file of these locations, and ask user if the required
      // location isn't listed.
      string directory = "/local/scratch/db434/workspace/lokiprefix/loki-elf/lib";
      string library = "sim.ld";
      string fullpath = directory + "/" + library;

      struct stat fileInfo;
      int failure = stat(fullpath.c_str(), &fileInfo);

      if(failure) {
        cerr << "Error: FileReader unable to access linker:\n  " << fullpath << endl;
        cerr << "Ask Alex for the latest version." << endl;
        tidy();
        throw std::exception();
      }

      // Build up the required command.
      string command = "loki-elf-ld -o ";

      linkedFile = filesToLink.back() + "_linked";  // Output file
      command += linkedFile + " ";

      for(unsigned int i=0; i<filesToLink.size(); i++) {
        command += filesToLink[i] + " ";
      }

      command += "-L" + directory;
      command += " -T" + library;

      // Execute the command.
      failure = system(command.c_str());
      if(failure) {
        cerr << "Error: unable to link files using command:\n  " << command << endl;
        tidy();
        throw std::exception();
      }

      // Generate a FileReader for the output ELF file.
      return new ELFFileReader(linkedFile, CORES_PER_TILE, 0);
    }
  }
}

/* Delete all of the temporary object files we created, so the filesystem is
 * kept tidy. There is no point in keeping them because they will be re-made
 * next time the simulator is run. */
void FileReader::tidy() {
  for(unsigned int i=0; i<filesToLink.size(); i++) {
    deleteFile(filesToLink[i]);
  }

  // Might like to keep this one so the linker isn't needed?
  if(linkedFile != string()) deleteFile(linkedFile);
}

/* Perform some small parameter-dependent transformations on the assembly
 * code. For example, change "ch1" to "r17". */
void FileReader::translateAssembly(string& infile, string& outfile) {
  std::ifstream in(infile.c_str());
  std::ofstream out(outfile.c_str());

  char line[200];   // An array of chars to load a line from the file into.

  while(!in.fail()) {
    in.getline(line, 200);

    try {
      // Try to make an instruction out of the line. In creating the
      // instruction, all of the transformations we want to do are performed.
      Instruction i(line);
      std::stringstream ss;
      ss << i;
      out << "\t" << ss.rdbuf()->str() << "\n";
    }
    catch(std::exception& e) {
      // If we couldn't make an instruction, just copy the line across.
      // It may be a label or some other important line.
      out << line << "\n";
    }
  }

  in.close();
  out.close();
}

void FileReader::deleteFile(string& filename) {
  int failure = remove(filename.c_str());
  if(failure) cerr << "Warning: unable to delete temporary file "
                   << filename << endl;
}

FileReader::FileReader(string& filename, ComponentID component, MemoryAddr position) {
  filename_ = filename;
  componentID_ = component;
  position_ = position;
}
