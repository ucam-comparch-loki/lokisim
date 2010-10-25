/*
 * CodeLoader.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "CodeLoader.h"
#include "DataBlock.h"
#include "FileReader.h"
#include "../Parameters.h"
#include "../StringManipulation.h"
#include "../../Datatype/Data.h"
#include "../../Datatype/Instruction.h"

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

      vector<string>& words = StringManipulation::split(s, ' ');

      if(words[0]=="directory") {     // Update the current directory
        directory = directory + "/" + words[1];
      }
      else if(words[0]=="loader") {   // Use another file loader
        string loaderFile = directory + "/" + words[1];
        loadCode(loaderFile, tile);
      }
      else {                          // Load code/data from the given file
        // If a full path is provided, use that. Otherwise, assume the file
        // is in the previously specified directory.
        words[1] = (words[1][0]=='/') ? words[1] : directory + "/" + words[1];

        loadCode(words, tile);
      }

      delete &words;

      if(file.eof()) break;
    }
    catch(std::exception& e) {
      std::cerr << "Error: could not read file " << settings << endl;
      break;
    }
  }

  file.close();

}

void CodeLoader::loadCode(vector<string>& command, Tile& tile) {
  FileReader& reader = FileReader::makeFileReader(command);
  vector<DataBlock>& blocks = reader.extractData();

  for(uint i=0; i<blocks.size(); i++) {
    tile.storeData(blocks[i].data(), blocks[i].component(),
                   blocks[i].position()/BYTES_PER_WORD);
    delete &(blocks[i].data());
  }

  delete &blocks;
  delete &reader;
}
