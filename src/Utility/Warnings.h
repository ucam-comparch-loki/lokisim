/*
 * Warnings.h
 *
 * Toggles for all warnings in the system.
 *
 *  Created on: 11 Aug 2015
 *      Author: db434
 */

#ifndef SRC_UTILITY_WARNINGS_H_
#define SRC_UTILITY_WARNINGS_H_

using std::string;

// Memory addresses which aren't aligned to a word/half-word boundary are
// automatically rounded down by the memory bank.
extern bool WARN_UNALIGNED;

inline void setWarning(const string& option) {
  string name;
  bool setting;

  string::size_type found = option.find("=");
  if (found == string::npos) {
    name = option;
    setting = true;
  }
  else {
    name = option.substr(0, found);
    string settingStr = option.substr(found+1);
    setting = (settingStr == "true") || (settingStr == "1");
  }

  if (name == "all" || name == "unaligned-memory")
    WARN_UNALIGNED = setting;
  else
    std::cout << "Unrecognised warning setting: " << name << " set to " << setting << std::endl;
}


#endif /* SRC_UTILITY_WARNINGS_H_ */
