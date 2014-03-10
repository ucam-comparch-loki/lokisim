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
  // of all substrings. The resultant vector should be deleted after use.
  static vector<string>& split(const string& s, char delim);

  // Split the given string around the delimiter character, putting the result
  // in the given vector.
  static void            split(const string& s, char delim, vector<string>& elems);

  // Convert the string to an integer.
  static int             strToInt(const string& s);

};

#endif /* STRINGMANIPULATION_H_ */
