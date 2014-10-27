/*
 * Callgrind.h
 *
 * Class used to generate a trace in a format compatible with Callgrind.
 *
 * For each function executed, need a block like:
 *   fl=<file name>
 *   fn=<function name>
 *   <line number> <events for this function>
 *   cfi=<file name of called function>
 *   cfn=<called function name>
 *   calls=<number of calls> <line in file>
 *   <line number> <events for this function>
 *
 * fn data only cares about functions called directly from this one
 * cfn data collapses all time spent in called functions
 *
 * More documentation available here:
 *   http://valgrind.org/docs/manual/cl-format.html
 *
 *  Created on: 24 Oct 2014
 *      Author: db434
 */

#ifndef CALLGRIND_H_
#define CALLGRIND_H_

#include <map>
#include "../../Datatype/ComponentID.h"
#include "../../Typedefs.h"

using std::ofstream;
using std::ostream;
using std::string;
using std::vector;

class Callgrind {

private:

  // Used to determine which function any given instruction is within.
  // TODO: including the file and line number would be useful.
  typedef struct {
    MemoryAddr start;
    MemoryAddr end;
    string     name;
  } FunctionInfo;

  // Information to be collected.
  struct Stats_ {
    count_t instructions;
    count_t cycles;

    void clear() {instructions = cycles = 0;}
    void operator+= (const struct Stats_& s) {
      instructions += s.instructions;
      cycles       += s.cycles;
    }
    friend ostream& operator<< (ostream& os, struct Stats_ const& s) {
      os << s.instructions << " " << s.cycles;
      return os;
    }
  };
  typedef struct Stats_ Stats;

  // Data on any function which has been called from another function.
  // Stats are *inclusive* of any functions which are called internally.
  typedef struct {
    count_t     timesCalled;
    Stats       stats;
  } CalledFunction;

  // Standalone data for each function.
  // Stats are *exclusive* of any called functions - the data is contained in
  // calledFunctions.
  typedef struct {
    Stats      stats;
    std::map<MemoryAddr, CalledFunction> calledFunctions;
  } TopLevelFunction;

public:

  // Open the named file and print a header. Also collect information from the
  // symbol table of the binary so later we can determine which function is
  // being executed at any time.
  static void startTrace(const string& logFile, const string& binaryFile);

  // Tidy up when execution has finished.
  static void endTrace();

  // Call this method to record that an instruction from a given address was
  // executed on a given cycle.
  static void instructionExecuted(ComponentID core, MemoryAddr address, cycle_count_t cycle);

  // Returns whether instructionExecuted may be called.
  static bool acceptingData();

private:

  // Return the function which contains the given instruction address.
  static const FunctionInfo& functionOf(MemoryAddr instAddress);

  // Extract information from all functions in the named binary.
  static void buildFunctionList(const string& binaryFile);

  // Call this method whenever the functions of the previous two instructions
  // differ.
  static void functionChanged(unsigned int coreID, MemoryAddr address, cycle_count_t cycle);

  // Record that one function called another.
  static void functionCall(MemoryAddr caller, MemoryAddr callee);

private:

  // Keep track of whether we are currently tracing.
  static bool tracing;

  // The stream to write all information to.
  static ofstream* log;

  // A sorted list of all functions in the program. Used to determine which
  // function a given instruction is within.
  static vector<FunctionInfo> functions;

  // Record the execution statistics for each function.
  static std::map<MemoryAddr, TopLevelFunction> functionStats;

  // Memory addresses showing which functions have called which.
  // Use a vector (not a stack) because we need to be able to iterate through.
  // One for each core.
  static vector<vector<MemoryAddr> > callStack;

  // Data for the function which is currently executing.
  // One for each core.
  static vector<Stats> currentFunction;

};

#endif /* CALLGRIND_H_ */
