/*
 * StringManipulation.h
 *
 * A class containing useful methods for manipulating strings.
 *
 *  Created on: 23 Mar 2010
 *      Author: db434
 */

#ifndef STRINGMANIPULATION_H_
#define STRINGMANIPULATION_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

class StringManipulation {

public:
  static vector<string>& split(const string& s, char delim);
  static int             strToInt(const string& s);

private:
  static vector<string>& split(const string& s, char delim, vector<string>& elems);

};

#endif /* STRINGMANIPULATION_H_ */
