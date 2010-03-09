/*
 * TraceFile.h
 *
 *  Created on: 5 Mar 2010
 *      Author: db434
 */

#ifndef TRACEFILE_H_
#define TRACEFILE_H_

#include "systemc"
#include "Parameters.h"

using sc_core::sc_trace_file;
using std::string;

class TraceFile {

public:
/* Methods */
  static sc_trace_file* initialiseTraceFile(char* name);

private:
  static string parameterSummary();

};

#endif /* TRACEFILE_H_ */
