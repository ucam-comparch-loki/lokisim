/*
 * InstrumentationBase.cpp
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#include <sstream>
#include "InstrumentationBase.h"
#include "../../Datatype/Flit.h"

using namespace Instrumentation;

int InstrumentationBase::popcount(uint value) {
  return __builtin_popcount(value);
}

int InstrumentationBase::popcount(const NetworkData& value) {
  return popcount(value.payload().toInt()) + popcount(value.channelID().flatten());
}

int InstrumentationBase::hammingDistance(uint val1, uint val2) {
  return __builtin_popcount(val1 ^ val2);
}

int InstrumentationBase::hammingDistance(const NetworkData& val1,
                                         const NetworkData& val2) {
  return hammingDistance(val1.payload().toInt(), val2.payload().toInt()) +
         hammingDistance(val1.channelID().flatten(), val2.channelID().flatten());
}

string InstrumentationBase::percentage(count_t value, count_t total) {
  double percentage = (total==0) ? 0.0 : (double)value/total * 100;
  std::stringstream ss;
  ss << std::fixed;
  ss.precision(1);
  ss << percentage << "%";
  return ss.str();
}

const string InstrumentationBase::xmlBegin(const char* name) {
  std::stringstream ss;
  ss << "<" << name << ">";
  return ss.str();
}

const string InstrumentationBase::xmlEnd(const char* name) {
  std::stringstream ss;
  ss << "</" << name << ">";
  return ss.str();
}

const string InstrumentationBase::xmlNode(const char* name, count_t value, const char* indent) {
  std::stringstream ss;
  std::noskipws(ss);
  ss << indent << xmlBegin(name) << value << xmlEnd(name);
  return ss.str();
}

void InstrumentationBase::init() {
  // Do nothing
}

void InstrumentationBase::end() {
  // Do nothing
}
