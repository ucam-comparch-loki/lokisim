/*
 * CodeLoader.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include <ios>
#include "CodeLoader.h"
#include "DataBlock.h"
#include "Parameters.h"
#include "StringManipulation.h"
#include "../Datatype/Data.h"
#include "../Datatype/Instruction.h"
#include "../TileComponents/TileComponent.h"
#include "../TileComponents/WrappedTileComponent.h"

bool CodeLoader::usingDebugger = false;

/* Use an external file to tell which files to read.
 * The file should contain lines of the following forms:
 *   directory directory_name
 *     Change the current directory
 *   loader file_name
 *     Use another file loader - useful for loading multiple (sub-)programs
 *   component_id file_name
 *     Load the contents of the file into the component */
void CodeLoader::loadCode(string& settings, Tile& tile) {

  char line[200];   // An array of chars to load a line from the file into.

  // Open the settings file.
  std::ifstream file(settings.c_str());

  // Strip away "/loader.txt" to get the directory path.
  int pos = settings.find("loader.txt");
  string directory = settings.substr(0,pos-1);

  while(!file.fail()) {
    try {
      file.getline(line, 200, '\n');
      string s(line);

      if(s[0]=='%' || s[0]=='\0') continue;   // Skip past any comments

      vector<string> words = StringManipulation::split(s, ' ');

      if(words[0]=="directory") {     // Update the current directory
        directory = directory + "/" + words[1];
      }
      else if(words[0]=="loader") {   // Use another file loader
        string loaderFile = directory + "/" + words[1];
        loadCode(loaderFile, tile);
      }
      else {                          // Load code/data from the given file
        int index = StringManipulation::strToInt(words[0]);
        // If a full path is provided, use that. Otherwise, assume the file
        // is in the previously specified directory.
        string filename = (words[1][0]=='/') ? words[1]
                                             : directory + "/" + words[1];
        loadCode(filename, tile, index);
      }

      if(file.eof()) break;
    }
    catch(std::exception& e) {
      std::cerr << "Error: could not read file " << settings << endl;
      break;
    }
  }

  file.close();

}

/* Load code from the specified file into a particular component of the
 * given tile. */
void CodeLoader::loadCode(string& filename, Tile& tile, uint component) {
  if(usingDebugger) cout << "\nLoading into " <<
      (component>=CLUSTERS_PER_TILE ? "memory " : "core ") << component << ":\n";

  vector<DataBlock>& blocks = getData(filename);

  for(uint i=0; i<blocks.size(); i++) {
    tile.storeData(blocks[i].data(), component, blocks[i].position()/BYTES_PER_WORD);
  }
}

/* Return a vector of Words corresponding to the contents of the file. */
vector<DataBlock>& CodeLoader::getData(string& filename) {

  // See if this file contains Instructions or Data
  int typeOfFile = fileType(filename);

  // Reading ELF files is more complicated, so we have a separate method for
  // that.
  if(typeOfFile == ELF) return readELFFile(filename);

  std::ifstream file(filename.c_str());
  vector<Word>* words = new vector<Word>();

  int wordsRead = 0;

  while(!file.fail()) {

    try {
      Word w = getWord(file, typeOfFile);
      words->push_back(w);

      if(usingDebugger && (typeOfFile != DATA)) {
        printInstruction((Instruction)w, wordsRead*BYTES_PER_WORD);
      }
    }
    catch (std::exception& e) {
      continue; // If we couldn't make a valid word, try the next line
    }

    wordsRead++;

    if(file.eof()) break;

  }

  if(words->size() == 0)
    std::cerr << "Error: read 0 words from file " << filename << endl;
  else if(DEBUG) cout << "Retrieved " << words->size() <<
    " words from file " << filename << endl;

  file.close();

  vector<DataBlock>* blocks = new vector<DataBlock>();
  blocks->push_back(DataBlock(words));

  return *blocks;

}

/* Returns whether the file should contain instructions. If not, it should
 * contain data. Instruction files are of type .loki, and data files are
 * of type .data. */
uint8_t CodeLoader::fileType(string& filename) {

  vector<string> parts = StringManipulation::split(filename, '.');

  if(parts.size() < 2) {
    // ELF files don't have a file extension.
    return ELF;
  }

  // There may be other '.' characters in the file name, but we only want the
  // final piece of the filename.
  int index = parts.size() - 1;

  if(parts[index] == "loki") return LOKI;
  else if(parts[index] == "data") return DATA;
  else if(parts[index] == "bloki") return BINARY;
  else std::cerr << "Unknown file format: " << filename << endl;

  return DATA;

}

/* Return either an Instruction or a Data from the file, depending on the
 * type of file being read. */
Word CodeLoader::getWord(std::ifstream& file, uint8_t type) {

  // An array to store individual lines of the file in.
  static char line[200];

  switch(type) {
    case BINARY: {
      // At the moment, binary files are formatted as 32-bit words in hex.
      // We therefore need to load in two words to make a single instruction.
      file.getline(line, 200, '\n');
      uint64_t val = (uint64_t)StringManipulation::strToInt(line) << 32;
      file.getline(line, 200, '\n');
      val += (uint64_t)StringManipulation::strToInt(line);
      return Instruction(val);
    }
    case DATA: {
      file.getline(line, 200, '\n');
      int val = StringManipulation::strToInt(line);
      return Data(val);
    }
    case ELF: {
      uint64_t result=0, byte=0;

      // Find a neater way to reverse endianness?
      byte = file.get();
      result |= byte << 32;
      byte = file.get();
      result |= byte << 40;
      byte = file.get();
      result |= byte << 48;
      byte = file.get();
      result |= byte << 56;

      byte = file.get();
      result |= byte << 0;
      byte = file.get();
      result |= byte << 8;
      byte = file.get();
      result |= byte << 16;
      byte = file.get();
      result |= byte << 24;

      return Instruction(result);
    }
    case LOKI: {
      file.getline(line, 200, '\n');
      return Instruction(line);
    }
    default: {
      std::cerr << "Error: unknown file format." << endl;
      throw std::exception();
    }
  }

}

vector<DataBlock>& CodeLoader::readELFFile(string& filename) {
  vector<DataBlock>* blocks = new vector<DataBlock>();

  // Open the input file.
  std::ifstream elfFile(filename.c_str());

  // Execute a command which returns information on all of the ELF sections.
  FILE* terminalOutput;
  string command("loki-elf-objdump -h " + filename);
  terminalOutput = popen(command.c_str(), "r");

  char line[100];

  // Step through each line of information, looking for ones of interest.
  while(!feof(terminalOutput)) {
    fgets(line, 100, terminalOutput);
    string lineStr(line);
    vector<string> words = StringManipulation::split(lineStr, ' ');

    // We're only interested in a few particular lines of the information.
    if(words.size() == 7 && (words[1]==".text" || words[1]==".data")) {
      string name      = words[1];
      int size         = StringManipulation::strToInt("0x"+words[2]);
//      int virtPosition = StringManipulation::strToInt("0x"+words[3]);
      int physPosition = StringManipulation::strToInt("0x"+words[4]);
      int offset       = StringManipulation::strToInt("0x"+words[5]);

      // Seek to "offset" in elfFile.
      elfFile.seekg(offset, std::ios::beg);

      vector<Word>* instructions = new vector<Word>();

      // Read in "size" bytes (remembering that each instruction is 8 bytes).
      for(int i=0; i<size; i+=8) {
        Instruction inst = getWord(elfFile, ELF);
        instructions->push_back(inst);

        if(usingDebugger) printInstruction(inst, physPosition+i);
      }

      // Put these instructions into a particular position in memory.
      DataBlock block(instructions, physPosition);
      blocks->push_back(block);
    }
  }

  fclose(terminalOutput);
  elfFile.close();

  return *blocks;
}

/* Print an instruction in the form:
 *   [position] instruction
 * in such a way that everything aligns nicely. */
void CodeLoader::printInstruction(Instruction i, int address) {
  // Set up some output formatting so numbers are all the same length.
  cout.fill('0');

  cout << "\[";
  cout.width(4);
  cout << std::right << address << "]\t" << i << endl;

  // Undo the formatting changes.
  cout.fill(' ');
}
