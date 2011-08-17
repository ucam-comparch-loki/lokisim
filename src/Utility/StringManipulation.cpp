/*
 * StringManipulation.cpp
 *
 *  Created on: 23 Mar 2010
 *      Author: db434
 */

#include <iostream>
#include <sstream>
#include "StringManipulation.h"

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
  string copy = str;

  if(str[1] == 'x') {
    ss << std::hex;  // Working with hex
    copy.erase(0,2);
  }

  // See if the string actually represents a number.
  for(unsigned int i=0; i<copy.length(); i++) {
    if(!std::isxdigit(copy[i]) && !std::isspace(copy[i]) && copy[i] != '-' && copy[i] != ',') {
//      std::cerr << "Not a number: " << copy << std::endl;
      throw std::exception();
    }
  }

  ss << copy;

  unsigned int num;
  ss >> num;
  return num;
}
