/*
 * InstrumentationBase.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "InstrumentationBase.h"
#include <sstream>

int InstrumentationBase::popcount(uint value) {
  return __builtin_popcount(value);
}

int InstrumentationBase::hammingDistance(uint val1, uint val2) {
  return __builtin_popcount(val1 ^ val2);
}

string InstrumentationBase::percentage(count_t value, count_t total) {
  double percentage = (total==0) ? 0.0 : (double)value/total * 100;
  std::stringstream ss;
  ss.precision(3);
  string s;
  ss << percentage << "%";
  ss >> s;
  return s;
}

void InstrumentationBase::init() {
  // Do nothing
}

void InstrumentationBase::end() {
  // Do nothing
}
