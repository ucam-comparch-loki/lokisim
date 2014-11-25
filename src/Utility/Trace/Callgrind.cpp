/*
 * Callgrind.cpp
 *
 *  Created on: 24 Oct 2014
 *      Author: db434
 */

#include "Callgrind.h"
#include "../Instrumentation.h"
#include "../Parameters.h"
#include "../StringManipulation.h"

using std::cout;
using std::cerr;
using std::endl;
using std::map;

bool                                              Callgrind::tracing = false;
ofstream*                                         Callgrind::log;
vector<Callgrind::FunctionInfo>                   Callgrind::functions;
Callgrind::FunctionInfo                           Callgrind::unknownFunction;
map<MemoryAddr, Callgrind::TopLevelFunction>      Callgrind::functionStats;
vector<vector<MemoryAddr> >                       Callgrind::callStack;
vector<Callgrind::Stats>                          Callgrind::currentFunction;

void Callgrind::startTrace(const string& logfile, const string& binaryFile) {
  // Open the log file and print the header to it.
  log = new ofstream(logfile.c_str());
  *log << "events: Cycles Instructions" << "\n\n";
  // Could also add in cache information?

  // Collect all relevant information from the binary.
  buildFunctionList(binaryFile);

  unknownFunction.start = 0;
  unknownFunction.end = -1;
  unknownFunction.name = "unknown";

  for (unsigned int i=0; i<NUM_CORES; i++) {
    callStack.push_back(vector<MemoryAddr>());
    callStack[i].push_back(0);
    currentFunction.push_back(Stats());
  }

  tracing = true;
}

void Callgrind::endTrace() {
  if (!tracing)
    return;

  // Flush out any remaining data.
  // Use functionChanged, pretending that the final function has returned.
  cycle_count_t cycle = Instrumentation::currentCycle();
  for (unsigned int core=0; core<NUM_CORES; core++) {
    if (callStack[core].size() >= 2) {
      MemoryAddr caller = callStack[core][callStack[core].size()-2];
      functionChanged(core, caller, cycle);
    }
  }

  // Print out all stats.
  // TODO: there's some placeholder information in here - remove it.
  for (map<MemoryAddr,TopLevelFunction>::iterator it=functionStats.begin(); it!=functionStats.end(); ++it) {
    MemoryAddr address = it->first;
    TopLevelFunction& data = it->second;
    *log << "fl=" << "somefile.c" << "\n";
    *log << "fn=" << functionOf(address).name << "\n";
    *log << 0 << " " << data.stats << "\n";

    for (map<MemoryAddr,CalledFunction>::iterator it2=data.calledFunctions.begin(); it2!=data.calledFunctions.end(); ++it2) {
      MemoryAddr address2 = it2->first;
      const CalledFunction& data2 = it2->second;
      *log << "cfi=" << "somefile.c" << "\n";
      *log << "cfn=" << functionOf(address2).name << "\n";
      *log << "calls=" << data2.timesCalled << " " << 0 << "\n";
      *log << 0 << " " << data2.stats << "\n";
    }

    *log << "\n";
  }

  // May optionally like to print a footer here.
  // e.g. A summary of total execution counts.

  // Close the stream.
  log->close();
  delete log;
}

void Callgrind::instructionExecuted(ComponentID core, MemoryAddr address, cycle_count_t cycle) {
  assert(tracing);

  unsigned int id = core.getGlobalCoreNumber();

  // Determine whether this is a continuation of the same function, or whether
  // we are in a new one.
  const FunctionInfo& fn = functionOf(address);

  if (fn.start != callStack[id].back())
    functionChanged(id, fn.start, cycle);

  // Increment any counters.
  currentFunction[id].instructions++;
}

bool Callgrind::acceptingData() {
  return tracing;
}

const Callgrind::FunctionInfo& Callgrind::functionOf(MemoryAddr instAddress) {
  for (unsigned int i=0; i<functions.size(); i++) {
    if (instAddress >= functions[i].start &&
        instAddress <  functions[i].end) {
      return functions[i];
    }
  }

  cerr << "Callgrind tracer unable to find function containing address 0x"
       << std::hex << instAddress << std::dec << endl;

  return unknownFunction;
}

void Callgrind::buildFunctionList(const string& binaryFile) {
  // Get the symbol table from the binary using loki-elf-objdump -t.
  string command("loki-elf-objdump -t " + binaryFile);
  FILE* terminalOutput = popen(command.c_str(), "r");

  // Format of a line:
  //   00002588 g     F .text  00000bec decode
  // line[0:7]    = start address
  // line[9:15]   = flags
  // line[17:22]  = segment (text, data, read-only, etc)
  // line[23:30]  = length (bytes)
  // line[32:]    = name
  char lineBuf[100];
  while (fgets(lineBuf, 100, terminalOutput) != NULL) {
    const string line(lineBuf);
    // We're only interested in the functions: in the text segment.
    if (line.length() > 34 && line.substr(17,5) == ".text") {
      std::stringstream ss, ss2;
      FunctionInfo info;

      // Trim the trailing newline from the name.
      info.name = line.substr(32, line.size()-32-1);

      // Start address
      ss << std::hex << line.substr(0,8);
      ss >> info.start;

      // Length.
      unsigned int length;
      ss2 << std::hex << line.substr(24,8);
      ss2 >> length;
      if (length == 0)
        continue;

      // Add length to start to get end of function.
      info.end = info.start + length;

      functions.push_back(info);
    }
  }

  // Add a final special-case function.
  FunctionInfo info;
  info.start = 0;
  info.end = 4;
  info.name = "bootloader";
  functions.push_back(info);
}

void Callgrind::functionChanged(unsigned int core, MemoryAddr address, cycle_count_t cycle) {
  // Flush the statistics for the current function. This includes adding to the
  // function's top-level statistics store, and incrementing the time spent in
  // every function in the call stack.
  currentFunction[core].cycles = cycle - currentFunction[core].cycles;
  functionStats[callStack[core].back()].stats += currentFunction[core];
  for (unsigned int i=0; i<callStack[core].size()-1; i++) {
    MemoryAddr caller = callStack[core][i];
    MemoryAddr callee = callStack[core][i+1];
    functionStats[caller].calledFunctions[callee].stats += currentFunction[core];
  }

  // Determine whether this was a function call or a function return by
  // comparing with the previous function.
  MemoryAddr prev = callStack[core].back();
  callStack[core].pop_back();
  bool functionCalled = address != callStack[core].back();

  // If function call, add new slot to the call stack.
  if (functionCalled) {
    callStack[core].push_back(prev);
    callStack[core].push_back(address);
    functionCall(prev, address);
  }

  // Update the current function.
  currentFunction[core].clear();
  currentFunction[core].cycles = cycle;  // Reuse cycles field to store start time.
}

void Callgrind::functionCall(MemoryAddr caller, MemoryAddr callee) {
  // Ensure that caller's top-level information includes an entry for callee,
  // and increment the number of times it has been called.
  TopLevelFunction& callerStats = functionStats[caller];
  callerStats.calledFunctions[callee].timesCalled++;
}
