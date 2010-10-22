/*
 * StringManipulation.cpp
 *
 *  Created on: 23 Mar 2010
 *      Author: db434
 */

#include "StringManipulation.h"
#include <sstream>

/* Split a string around a given delimiter character. */
vector<string>& StringManipulation::split(const string& s, char delim,
                                          vector<string>& elems) {
  std::stringstream ss(s);
  string item;
  while(std::getline(ss, item, delim)) {
    if(item.length() > 0) elems.push_back(item);
  }
  return elems;
}

/* Split a string around a given delimiter character. The resultant vector
 * should be deleted when finished with. */
vector<string>& StringManipulation::split(const string& s, char delim) {
  vector<string>* elems = new vector<string>();
  return split(s, delim, *elems);
}

/* Return the integer represented by the given string. */
int StringManipulation::strToInt(const string& str) {
  std::stringstream ss;

  if(str[1] == 'x') ss << std::hex << str;  // Working with hex
  else ss << str;

  unsigned int num;
  ss >> num;
  return num;
}
