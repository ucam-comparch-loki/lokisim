/*
 * Arguments.h
 *
 * Parse and store the command line arguments so they can be accessed at any
 * time.
 *
 *  Created on: 3 Apr 2012
 *      Author: db434
 */

#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

class Chip;

class Arguments {

public:

  // Parse all command line arguments. Some will be dealt with immediately, and
  // some will be stored for later use.
  static void parse(int argc, char* argv[]);

  // Take any arguments which are for the simulated program, and store them in
  // their initial positions in simulated memory.
  // These arguments are separated from the simulator arguments by "--args".
  static void storeArguments(Chip& chip);

  // Tells whether simulation should actually take place.
  static bool simulate();

  // A list of files containing code to be simulated.
  static const vector<string>& code();
  static const string& energyTraceFile();

  // The command used to run the simulator.
  static const string invocation();

  static void printHelp();

private:

  // Tells whether we actually want to simulate the chip.
  // Consider extracting all non-simulation functionality out into separate
  // programs?
  static bool simulate_;

  // The subset of the simulator's command line arguments which are to be
  // passed to the simulated program.
  static unsigned int programArgc;
  static char** programArgv;

  // A list of files containing code the simulator should execute.
  static vector<string> programFiles;

  // Filenames used for dumping information.
  static string coreTraceFile_, memTraceFile_, energyTraceFile_, softwareTraceFile_;

  // The command used to run the simulator.
  static std::stringstream invocation_;

};

#endif /* ARGUMENTS_H_ */
