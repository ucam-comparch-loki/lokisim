/*
 * InstrumentationBase.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include "InstrumentationBase.h"
#include <sstream>

string InstrumentationBase::asPercentage(int value, int total) {
  double percentage = (double)value/total * 100;
  std::stringstream ss;
  ss.precision(3);
  string s;
  ss << percentage << "%";
  ss >> s;
  return s;
}
