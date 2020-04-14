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
#include "../Config.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

using std::cout;
using std::cerr;
using std::endl;

vector<string> FileReader::filesToLink;
vector<string> FileReader::tempFiles;
string FileReader::linkedFile;
bool FileReader::foundAsmFile = false;

const int BACKGROUND_MEMORY = -1;
const int FIRST_CORE = 0;

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

FileReader* FileReader::makeFileReader(const vector<string>& words,
    bool customAppLoader) {
  FileReader* reader;

  // Some sensible defaults.
  string filename = words[0];
  int component = BACKGROUND_MEMORY;
  int position = 0x1000;

  if (words.size() == 1) {
    // Assume that we want the code to go into the background memory and that
    // the first core should execute it.

    // Defaults are fine.
  } else if (words.size() == 2 && words[0] == "apploader") {
    component = FIRST_CORE;
    position = 0;
    filename = words[1];
  } else if (words.size() == 2) {
    component = StringManipulation::strToInt(words[1]);
  } else if (words.size() == 3) {
    component = StringManipulation::strToInt(words[1]);
    position = StringManipulation::strToInt(words[2]);
  } else {
    cerr << "Error: invalid loader file line:\n  ";
    for (unsigned int i = 0; i < words.size(); i++)
      cerr << words[i] << " ";
    cerr << endl;

    tidy();
    throw std::exception();
  }

  // Make sure file exists.
  if (!exists(filename)) {
    cerr << "Error: could not find input file " << filename << endl;
    tidy();
    throw std::exception();
  }

  // Split the filename into the name and the extension.
  int dotPos = filename.find_last_of('.');

  string name, extension;

  // If we found two dots in a row, we didn't find the file extension.
  // Similarly if we've found "./filename"
  if (filename[dotPos - 1] == '.' || filename[dotPos + 1] == '/') {
    name = filename;
    extension = "";
  } else {
    name = filename.substr(0, dotPos);
    extension = filename.substr(dotPos + 1);
  }

  ComponentID id(0,0,0);
  if (component != BACKGROUND_MEMORY) {
    // Deprecate loading programs into arbitrary cores?
    assert(component == FIRST_CORE);
    id = ComponentID(1, 1, 0);
  }

  if (extension == "" || extension == name) {
    filesToLink.push_back(name);
    reader = NULL;
  } else if (extension == "loki") {
    reader = new LokiFileReader(filename, id, position);
  } else if (extension == "data") {
    reader = new DataFileReader(filename, id, position);
  } else if (extension == "s") {
    string asmFile, elfFile;

    // Add a salt to temporary filenames so there aren't conflicts if multiple
    // instances of the simulator run at once.
    int salt = getpid();
    std::stringstream uniqueName;
    uniqueName << name << salt;
    uniqueName >> elfFile;
    asmFile = elfFile + ".s";

    // Parse any assembly notation which is parameter-dependent: e.g. (12,4),
    // and create a file which can be passed to the proper assembler.
    translateAssembly(filename, asmFile);

    string makeELF = "loki-elf-as " + asmFile + " -o " + elfFile;

    int returnCode = system(makeELF.c_str());
    if (returnCode != 0) {
      cerr << "Error: unable to assemble file using command:\n  " << makeELF
          << endl;
      tidy();
      throw std::exception();
    }

    deleteFile(asmFile);

    if (component == BACKGROUND_MEMORY) {
      filesToLink.push_back(elfFile);
      tempFiles.push_back(elfFile);
      foundAsmFile = true;
      reader = NULL;
    } else {
      reader = new ELFFileReader(elfFile, id, id, 0);
    }
  } else {
    // Assume the dot was just part of the filename/path
    filesToLink.push_back(filename);
    reader = NULL;
//    cerr << "Unknown file format: " << filename << endl;
//    tidy();
//    throw std::exception();
  }

  return reader;
}

FileReader* FileReader::linkFiles() {
  switch (filesToLink.size()) {
    case 0:
      return NULL;
    case 1: {
      // Don't need to link a single ELF file.
      if (!foundAsmFile)
        return new ELFFileReader(filesToLink[0], ComponentID(2,0,0),
            ComponentID(1, 1, 0), 0x1000);
      // Else, if an assembly file was found, drop through to the "default" and
      // invoke the linker. This will ensure the code is put in the correct
      // position in memory.
    }
      // no break
    default: {
      string directory = Config::getAttribute("lokiprefix",
          "location of lokiprefix (compilation tools)") + "/loki-elf/lib";
      string library = "sim.ld";
      string fullpath = directory + "/" + library;

      // Make sure the linking library exists.
      if (!exists(fullpath)) {
        // The location could be wrong: ask the user to try again?
        cerr << "Error: FileReader unable to access linker:\n  " << fullpath
            << endl;
        tidy();
        throw std::exception();
      }

      // Build up the required command.
      string command = "loki-elf-ld -o ";

      linkedFile = filesToLink.back() + "_linked";  // Output file
      command += linkedFile + " ";

      for (unsigned int i = 0; i < filesToLink.size(); i++) {
        command += filesToLink[i] + " ";
      }

      command += "-L" + directory;
      command += " -T" + library;

      // Execute the command.
      int returnCode = system(command.c_str());
      if (returnCode != 0) {
        cerr << "Error: unable to link files using command:\n  " << command
            << endl;
        linkedFile = "";
        tidy();
        throw std::exception();
      }

      // Generate a FileReader for the output ELF file.
      return new ELFFileReader(linkedFile, ComponentID(2,0,0), ComponentID(1,1,0),
          0x1000);
    }
  }
}

bool FileReader::exists(const string& filename) {
  return access(filename.c_str(), F_OK) == 0;
}

/* Delete all of the temporary object files we created, so the filesystem is
 * kept tidy. There is no point in keeping them because they will be re-made
 * next time the simulator is run. */
void FileReader::tidy() {
  for (unsigned int i = 0; i < tempFiles.size(); i++) {
    deleteFile(tempFiles[i]);
  }

  // Might like to keep this one so the linker isn't needed?
  if (linkedFile != string())
    deleteFile(linkedFile);
}

/* Perform some small parameter-dependent transformations on the assembly
 * code. For example, change "ch1" to "r17". */
void FileReader::translateAssembly(string& infile, string& outfile) {
  std::ifstream in(infile.c_str());
  std::ofstream out(outfile.c_str());

  char line[200];   // An array of chars to load a line from the file into.

  while (!in.fail()) {
    in.getline(line, 200);

    try {
      // Try to make an instruction out of the line. In creating the
      // instruction, all of the transformations we want to do are performed.
      Instruction i(line);
      std::stringstream ss;
      ss << i;
      out << "\t" << ss.rdbuf()->str() << "\n";
    } catch (std::exception& e) {
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
  if (failure)
    cerr << "Warning: unable to delete temporary file " << filename << endl;
}

FileReader::FileReader(const string& filename, const ComponentID& component,
    const MemoryAddr position) :
    filename_(filename), componentID_(component), position_(position) {

  // Do nothing

}
