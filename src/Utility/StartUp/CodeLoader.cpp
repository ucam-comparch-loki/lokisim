/*
 * CodeLoader.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "CodeLoader.h"
#include "DataBlock.h"
#include "FileReader.h"
#include "ELFFileReader.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Instruction.h"

bool CodeLoader::appLoaderInitialized = false;
int CodeLoader::mainOffset = -1;

/* Use an external file to tell which files to read.
 * The file should contain lines of the following forms:
 *   directory directory_name
 *     Change the current directory
 *   loader file_name
 *     Use another file loader - useful for loading multiple (sub-)programs
 *   parameter parameter_name parameter_value
 *     Override default parameter setting */
void CodeLoader::loadParameters(const string& settings) {

  // Check that this is actually a settings file.
  if (settings.find(".txt") == settings.size()) return;

  char line[200];   // An array of chars to load a line from the file into.

  // Open the settings file.
  std::ifstream file(settings.c_str());

  if (file.fail())
    std::cerr << "Warning: unable to open file " << settings << " while parsing parameters" << endl;

  // Strip away "/loader.txt" to get the directory path.
  int pos = settings.find("loader.txt");
  string directory = settings.substr(0,pos-1);

  while (!file.fail()) {
    try {
      file.getline(line, 200, '\n');
      string s(line);

      if (s[0]=='%' || s[0]=='\0') continue;   // Skip past any comments

      vector<string>& words = StringManipulation::split(s, ' ');
      if (words.size() == 0) continue;

      if (words[0]=="directory") {     // Update the current directory
        directory = directory + "/" + words[1];
      }
      else if (words[0]=="loader") {   // Use another file loader
        string loaderFile = directory + "/" + words[1];
        loadParameters(loaderFile);
      }
      else if (words[0]=="parameter") {   // Override parameter
        Parameters::parseParameter(words[1], words[2]);
      }

      delete &words;

      if (file.eof()) break;
    }
    catch (std::exception& e) {
      std::cerr << "Error: could not read file " << settings << " while parsing parameters" << endl;
      continue;
    }
  }

  file.close();
}

/* Use an external file to tell which files to read.
 * The file should contain lines of the following forms:
 *   directory directory_name
 *     Change the current directory
 *   loader file_name
 *     Use another file loader - useful for loading multiple (sub-)programs
 *   component_id file_name
 *     Load the contents of the file into the component */
void CodeLoader::loadCode(const string& settings, Chip& chip) {

  char line[200];   // An array of chars to load a line from the file into.

  // Open the settings file.
  std::ifstream file(settings.c_str());

  if (file.fail())
    std::cerr << "Warning: could not read file " << settings << " while parsing commands" << endl;

  // Strip away "/loader.txt" to get the directory path.
  unsigned int pos = settings.find(".txt");

  // If the line doesn't specify a text file, it is probably an executable.
  if (pos >= settings.length()) {
    // Put the string in a vector so we can use existing methods.
    vector<string> vec;
    vec.push_back(settings);
    loadFromCommand(vec, chip, false);
    return;
  }

  // Find out which directory the file is in.
  pos = settings.rfind("/");
  string directory = ((int)pos == -1) ? "" : (settings.substr(0,pos) + "/");

  while (!file.fail()) {
    try {
      file.getline(line, 200, '\n');
      string s(line);

      if (s[0]=='#' || s[0]=='\0') continue;   // Skip past any comments

      vector<string>& words = StringManipulation::split(s, ' ');

      if (words[0]=="directory") {     // Update the current directory
        directory = directory + words[1];
      }
      else if (words[0]=="loader") {   // Use another file loader
        string loaderFile = directory + words[1];
        loadCode(loaderFile, chip);
      }
      else if (words[0]=="parameter") {
        // Do nothing: parameters are dealt with in loadParameters()
      }
      else if (words[0]=="apploader") {
        // Load application loader code from the given file

        // Add the current directory onto the filename.
        words[1] = directory + words[1];
        if (!appLoaderInitialized)
          loadFromCommand(words, chip, true);
      }
      else {                          // Load code/data from the given file
        // If a full path is provided, use that. Otherwise, assume the file
        // is in the previously specified directory.
        //std::string filename = words.back();
        //words.back() = (filename[0]=='/') ? filename : directory + "/" + filename;

        // Add the current directory to the filename. Assumes the first word
        // is a filename.
        words[0] = directory + words[0];

        loadFromCommand(words, chip, false);
      }

      delete &words;

      if (file.eof()) break;
    }
    catch (std::exception& e) {
      std::cerr << "Error: could not read file " << settings << " while parsing commands" << endl;
      continue;
    }
  }

  file.close();

}

void CodeLoader::makeExecutable(Chip& chip) {
  FileReader* reader = FileReader::linkFiles();
  loadFromReader(reader, chip);

  // Now that the whole program is in simulated memory, any temporary program
  // files can be deleted.
  FileReader::tidy();

  if (!appLoaderInitialized) {
//    assert(mainOffset >= 0);
//    DataBlock &block = ELFFileReader::loaderProgram(ComponentID(0, 0), mainOffset);
//    chip.storeData(block.data(), block.component(), block.position());
//    delete &block;

    appLoaderInitialized = true;
  }
}

void CodeLoader::loadFromCommand(const vector<string>& command, Chip& chip, bool customAppLoader) {
  FileReader* reader = FileReader::makeFileReader(command, customAppLoader);
  loadFromReader(reader, chip);
}

void CodeLoader::loadFromReader(FileReader* reader, Chip& chip) {
  if(reader == NULL) return;

  vector<DataBlock>& blocks = reader->extractData(mainOffset);

  for (uint i=0; i<blocks.size(); i++) {
    if (blocks[i].component().getTile() == 0 && blocks[i].component().getPosition() == 0 && blocks[i].position() == 0)
      appLoaderInitialized = true;

    chip.storeData(blocks[i]);
    delete &(blocks[i].payload());
  }

  delete &blocks;
  delete reader;
}
