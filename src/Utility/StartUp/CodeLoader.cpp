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
#include "../../Datatype/Instruction.h"

/* Use an external file to tell which files to read.
 * The file should contain lines of the following forms:
 *   directory directory_name
 *     Change the current directory
 *   loader file_name
 *     Use another file loader - useful for loading multiple (sub-)programs
 *   parameter parameter_name parameter_value
 *     Override default parameter setting */
void CodeLoader::loadParameters(string& settings) {

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
	        loadParameters(loaderFile);
	      }
	      else if(words[0]=="parameter") {   // Override parameter
	        Parameters::parseParameter(words[1], words[2]);
	      }

	      delete &words;

	      if(file.eof()) break;
	    }
	    catch(std::exception& e) {
	      std::cerr << "Error: could not read file " << settings << endl;
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
void CodeLoader::loadCode(string& settings, Chip& chip) {

  char line[200];   // An array of chars to load a line from the file into.

  // Open the settings file.
  std::ifstream file(settings.c_str());

  // Strip away "/loader.txt" to get the directory path.
//  uint pos = settings.find("loader");
  uint pos = settings.find(".txt");

  // If the line doesn't specify a text file, it is probably an executable.
  if(pos >= settings.length()) {
    // Put the string in a vector so we can use existing methods.
    vector<string> vec;
    vec.push_back(settings);
    loadCode(vec, chip);
    return;
  }

  // Find out which directory the file is in.
  pos = settings.rfind("/");
  string directory = settings.substr(0,pos);

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
        loadCode(loaderFile, chip);
      }
      else if(words[0]=="power") {
        Instrumentation::loadPowerLibrary(words[1]);
      }
      else if(words[0]=="parameter") {
        // Do nothing: parameters are dealt with in loadParameters()
      }
      else {                          // Load code/data from the given file
        // If a full path is provided, use that. Otherwise, assume the file
        // is in the previously specified directory.
        std::string filename = words.back();
        words.back() = (filename[0]=='/') ? filename : directory + "/" + filename;

        loadCode(words, chip);
      }

      delete &words;

      if(file.eof()) break;
    }
    catch(std::exception& e) {
      std::cerr << "Error: could not read file " << settings << endl;
      continue;
    }
  }

  file.close();

}

void CodeLoader::loadCode(vector<string>& command, Chip& chip) {
  FileReader& reader = FileReader::makeFileReader(command);
  vector<DataBlock>& blocks = reader.extractData();

  for(uint i=0; i<blocks.size(); i++) {
    chip.storeData(blocks[i].data(), blocks[i].component(),
                   blocks[i].position()/BYTES_PER_WORD);
    delete &(blocks[i].data());
  }

  delete &blocks;
  delete &reader;
}
