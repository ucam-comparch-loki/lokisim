/*
 * TraceFile.h
 *
 *  Created on: 5 Mar 2010
 *      Author: db434
 */

#ifndef TRACEFILE_H_
#define TRACEFILE_H_

#include "systemc"

using sc_core::sc_trace_file;
using std::string;

class TraceFile {

//==============================//
// Methods
//==============================//

public:

  // Generate and initialise a VCD trace file with the given name.
  static sc_trace_file* initialiseTraceFile(string& name);

private:

  static string parameterSummary();

};

#endif /* TRACEFILE_H_ */
