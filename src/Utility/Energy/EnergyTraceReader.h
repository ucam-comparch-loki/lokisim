/*
 * EnergyTraceReader.h
 *
 *  Created on: 3 Apr 2012
 *      Author: db434
 */

#ifndef ENERGYTRACEREADER_H_
#define ENERGYTRACEREADER_H_

#include <iostream>

class EnergyTraceReader {

//==============================//
// Methods
//==============================//

public:

  static void start(const std::string& filename);
  static void stop();

private:

  static void read();
  static void readSingleEvent();

//==============================//
// Local state
//==============================//

private:

  // The stream to read the trace from.
  static std::ifstream *input;

};

#endif /* ENERGYTRACEREADER_H_ */
