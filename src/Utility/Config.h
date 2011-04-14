/*
 * Config.h
 *
 * Class used to access simulator configuration settings. These may include:
 *   Necessary external tools
 *   Parameters
 *   Power numbers
 *   ...
 *
 *  Created on: 14 Apr 2011
 *      Author: db434
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <string>

using std::string;

class Config {

public:

  // Get the attribute with the given name. A description may be provided to
  // help the user give the attribute if it does not yet exist.
  static string& getAttribute(string name, string description=string());

private:

  // Read the configuration file, and populate the attribute map.
  static void readConfigFile();

  // Ask the user what the value of the named attribute should be, and add it
  // to the map and the configuration file.
  static void askUser(string& name, string& description);

  static void initialise();

  static std::map<string,string> attributes;
  static string fileLocation;

};

#endif /* CONFIG_H_ */
