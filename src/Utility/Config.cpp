/*
 * Config.cpp
 *
 *  Created on: 14 Apr 2011
 *      Author: db434
 */

#include "Config.h"
#include "StringManipulation.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>

std::map<string,string> Config::attributes;
string Config::fileLocation;

string& Config::getAttribute(string name, string description) {
  // First, check to see if the attribute is already in the map.
  if(attributes.find(name) != attributes.end()) {
    return attributes[name];
  }

  // If not, read all attributes in from the configuration file and try again.
  else {
    readConfigFile();
    if(attributes.find(name) != attributes.end()) {
      return attributes[name];
    }

    // If it isn't in the file either, ask the user what the setting should be.
    else {
      askUser(name, description);
      return attributes[name];
    }
  }
}

void Config::readConfigFile() {
  initialise();
  std::ifstream file(fileLocation.c_str());

  char line[200];
  while(!file.fail()) {
    file.getline(line, 200);
    string str(line);

    // Each line in the config file should consist of a name-attribute pair,
    // separated by a single space.
    vector<string>& words = StringManipulation::split(str, ' ');
    if(words.size() != 2) continue;

    attributes[words[0]] = words[1];
    delete &words;
  }

  file.close();
}

void Config::askUser(string& name, string& description) {
  std::cout << "Unable to find " << name << "\nPlease enter ";

  if(description == string()) std::cout << "value";
  else                        std::cout << description;

  std::cout << ":\n";

  string value;
  std::cin >> value;

  attributes[name] = value;

  // Append the new attribute to the configuration file.
  std::ofstream file(fileLocation.c_str(), std::ios_base::app);
  file << name << " " << value << "\n";
  file.close();
}

void Config::initialise() {
  static bool initialised = false;
  if(initialised) return;

  // Generate the full path to the configuration file.
  std::stringstream ss;
  ss << getenv("HOME") << "/.config/lokisim"; // Just the directory for the moment
  ss >> fileLocation;

  // If the directory .config/lokisim doesn't exist yet, create it.
  struct stat status;
  stat(fileLocation.c_str(), &status);
  if(!(status.st_mode & S_IFDIR)) {
    string command = "mkdir " + fileLocation;
    int failure = system(command.c_str());
    if(failure) std::cerr << "Warning: unable to create configuration file\n";
  }

  // Now that the directory definitely exists, update fileLocation to point to
  // the file itself.
  fileLocation += "/settings";

  initialised = true;
}
