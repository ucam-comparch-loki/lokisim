/*
 * Arguments.cpp
 *
 *  Created on: 3 Apr 2012
 *      Author: db434
 */

#include "Arguments.h"
#include "Debugger.h"
#include "StringManipulation.h"
#include "Trace/CoreTrace.h"
#include "Trace/MemoryTrace.h"
#include "Trace/SoftwareTrace.h"
#include "../Chip.h"

bool Arguments::simulate_ = true;

unsigned int Arguments::programArgc = 0;
char** Arguments::programArgv = NULL;

vector<string> Arguments::programFiles;

string Arguments::coreTraceFile_ = "";
string Arguments::memTraceFile_ = "";
string Arguments::energyTraceFile_ = "";
string Arguments::softwareTraceFile_ = "";
std::stringstream Arguments::invocation_;

void Arguments::parse(int argc, char* argv[]) {

  bool useDefaultSettings = true;

  for (int i=0; i<argc; i++)
    invocation_ << argv[i] << " ";

  if (argc == 1) {
    printHelp();
    simulate_ = false;
  }

  for (int i=1; i<argc; i++) {
    string argument(argv[i]);
    if (argument == "debug") {
      // Print out lots of information about execution.
      Debugger::usingDebugger = true;
      Debugger::mode = Debugger::DEBUGGER;
    }
    else if (argument == "test") {
      // Switch off all status reporting, so we only get the information we
      // want. This allows much faster testing.
      DEBUG = 0;
      Debugger::usingDebugger = true;
      Debugger::mode = Debugger::TEST;
    }
    else if (argument == "trace") {
      // Print out only the addresses of each instruction executed.
      DEBUG = 0;
      TRACE = 1;
    }
    else if (argument == "-run" || argument == "-settings") {
      // Command line way of choosing which program to run.
      DEBUG = 0;
      string filename(argv[i+1]);
      programFiles.push_back(filename);
      i++;  // Have used two arguments in this iteration.

      if (argument == "-settings")
        useDefaultSettings = false;
    }
    else if (argument == "-batch") {
      // Enable batch mode.
      DEBUG = 0;
      BATCH_MODE = 1;
    }
    else if (argument == "-coretrace") {
      // Enable core trace.
      coreTraceFile_ = string(argv[i+1]);
      CoreTrace::start(coreTraceFile_);
      i++;  // Have used two arguments in this iteration.
      CORE_TRACE = 1;
    }
    else if (argument == "-memtrace") {
      // Enable memory trace.
      memTraceFile_ = string(argv[i+1]);
      MemoryTrace::start(memTraceFile_);
      i++;  // Have used two arguments in this iteration.
      MEMORY_TRACE = 1;
    }
    else if (argument == "-energytrace") {
      energyTraceFile_ = string(argv[i+1]);
      ENERGY_TRACE = 1;
      Instrumentation::startLogging();
      i++;  // Have used two arguments in this iteration.
    }
    else if (argument == "-swtrace") {
      softwareTraceFile_ = string(argv[i+1]);
      SOFTWARE_TRACE = 1;
      SoftwareTrace::start(softwareTraceFile_);
      i++;  // Have used two arguments in this iteration.
    }
    else if (argument == "--args") {
      // Pass command line options to the simulated program.
      programArgc = argc - i;
      programArgv = argv + i;
      break;
    }
    else if (argument == "--help") {
      printHelp();
      simulate_ = false;
      break;
    }
    else if (argument[0] == '-' && argument[1] == 'P') {
      // Use "-Pparam=value" to set a parameter on the command line.
      string parameter = argument.substr(2);
      vector<string>& parts = StringManipulation::split(parameter, '=');
      assert(parts.size() == 2);
      Parameters::parseParameter(parts[0], parts[1]);
      delete &parts;
    }
    else {
      cerr << "Warning: unrecognised argument: " << argument << endl;
    }
  }

  if (useDefaultSettings) {
    const string simPath = string(argv[0]);
    const string simDir = simPath.substr(0, simPath.rfind('/'));
    string settingsFile = simDir + "/../test_files/loader.txt";
    programFiles.push_back(settingsFile);
  }

}

// Store any relevant arguments in the Loki memory.
// Borrowed from Alex, in turn borrowed from moxie backend.
void Arguments::storeArguments(Chip& chip) {
  uint i, j, tp;

  /* Target memory looks like this:
     0x00000000 zero word
     0x00000004 argc word
     0x00000008 start of argv
     .
     0x0000???? end of argv
     0x0000???? zero word
     0x0000???? start of data pointed to by argv  */

  chip.writeWord(ComponentID(), 0, Word(0));
  chip.writeWord(ComponentID(), 4, Word(programArgc));

  /* tp is the offset of our first argv data.  */
  tp = 4 + 4 + programArgc*4 + 4;

  for (i = 0; i < programArgc; i++)
  {
    /* Set the argv value.  */
    chip.writeWord(ComponentID(), 4 + 4 + i*4, Word(tp));

    /* Store the string.  */
    for (j=0; j < (strlen(programArgv[i])+1); j++) {
      chip.writeByte(ComponentID(), tp+j, Word(programArgv[i][j]));
    }
    tp += strlen(programArgv[i]) + 1;
  }

  chip.writeWord(ComponentID(), 4 + 4 + i*4, Word(0));
}

bool Arguments::simulate() {
  return simulate_;
}

const vector<string>& Arguments::code() {
  return programFiles;
}

const string& Arguments::energyTraceFile() {
  return energyTraceFile_;
}

const string Arguments::invocation() {
  return invocation_.str();
}

void Arguments::printHelp() {
  cout <<
    "Loki2: a cycle-accurate simulator for the Loki many-core architecture.\n"
    "Usage: Loki2 [simulator arguments]\n\n"
    "Options:\n"
    "  debug\n\tEnter debug mode, where simulator contents can be inspected and changed\n\tat runtime\n"
    "  trace\n\tPrint the address of each instruction executed to stdout\n"
    "  -run <program>\n\tExecute the supplied program\n"
    "  -coretrace <file>\n"
    "  -memtrace <file>\n\tDump particular types of information to a named file\n"
    "  -Pparameter=value\n\tSet a named parameter to a particular value\n"
    "  --args [...]\n\tPass all remaining arguments to the simulated program\n"
    "  --help\n\tDisplay this information and exit\n"
    << endl;
}
