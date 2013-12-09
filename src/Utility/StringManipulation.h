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

  // Split the given string around the delimiter character, and return a vector
  // of all substrings.
  static vector<string>& split(const string& s, char delim);

  // Convert the string to an integer.
  static int             strToInt(const string& s);

private:
  static void            split(const string& s, char delim, vector<string>& elems);

};

#endif /* STRINGMANIPULATION_H_ */
