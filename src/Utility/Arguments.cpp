/*
 * Arguments.cpp
 *
 *  Created on: 3 Apr 2012
 *      Author: db434
 */

#include "Arguments.h"
#include "Debugger.h"
#include "StringManipulation.h"
#include "StartUp/DataBlock.h"
#include "Trace/Callgrind.h"
#include "Trace/CoreTrace.h"
#include "Trace/MemoryTrace.h"
#include "Trace/SoftwareTrace.h"
#include "Trace/LBTTrace.h"
#include "../Chip.h"

bool Arguments::simulate_ = true;
string Arguments::simulator_ = "";

unsigned int Arguments::programArgc = 0;
char** Arguments::programArgv = NULL;

vector<string> Arguments::programFiles;

string Arguments::coreTraceFile_ = "";
string Arguments::memTraceFile_ = "";
string Arguments::energyTraceFile_ = "";
string Arguments::softwareTraceFile_ = "";
string Arguments::lbtTraceFile_ = "";
string Arguments::stallsTraceFile_ = "";
string Arguments::callgrindTraceFile_ = "";
bool Arguments::csimTrace_ = false;
bool Arguments::instructionTrace_ = false;
bool Arguments::instructionAddressTrace_ = false;

bool Arguments::batchMode_ = false;

std::stringstream Arguments::invocation_;

bool Arguments::summarise_ = false;
bool Arguments::silent_ = false;

vector<string> Arguments::parameterNames;
vector<string> Arguments::parameterValues;

void Arguments::parse(int argc, char* argv[]) {

  bool useDefaultSettings = true;

  for (int i=0; i<argc; i++)
    invocation_ << argv[i] << " ";

  // Get the full simulator path, not just the name used on the command line.
  // This is Linux-specific.
  char buff[512];
  uint chars = readlink("/proc/self/exe", buff, sizeof(buff));
  if (chars > 0)
    simulator_ = string(buff).substr(0, chars);
  else
    assert(false && "Unable to determine simulator location.");


  if (argc == 1) {
    printHelp();
    simulate_ = false;
  }

  for (int i=1; i<argc; i++) {
    string argument(argv[i]);
    if (argument == "-debug") {
      // Print out lots of information about execution.
      Debugger::usingDebugger = true;
      Debugger::mode = Debugger::DEBUGGER;
    }
    else if (argument == "-test") {
      // Switch off all status reporting, so we only get the information we
      // want. This allows much faster testing.
      DEBUG = 0;
      Debugger::usingDebugger = true;
      Debugger::mode = Debugger::TEST;
    }
    else if (argument == "-trace") {
      // Print each instruction executed and its context.
      DEBUG = 0;
      csimTrace_ = true;
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
      batchMode_ = true;
    }
    else if (argument == "-instaddrtrace") {
      // Print out only the addresses of each instruction executed.
      DEBUG = 0;
      instructionAddressTrace_ = true;
    }
    else if (argument == "-insttrace") {
      // Print out the textual form of each instruction executed.
      DEBUG = 0;
      instructionTrace_ = true;
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
    }
    else if (argument == "-energytrace") {
      energyTraceFile_ = string(argv[i+1]);
      ENERGY_TRACE = 1;
      i++;  // Have used two arguments in this iteration.
    }
    else if (argument == "-swtrace") {
      softwareTraceFile_ = string(argv[i+1]);
      SoftwareTrace::start(softwareTraceFile_);
      i++;  // Have used two arguments in this iteration.
    }
    else if (argument == "-lbttrace") {
      lbtTraceFile_ = string(argv[i+1]);
      LBTTrace::start(lbtTraceFile_);
      i++;  // Have used two arguments in this iteration.
    }
    else if (argument == "-stalltrace") {
      stallsTraceFile_ = string(argv[i+1]);
      i++;  // Have used two arguments in this iteration.
      Instrumentation::Stalls::startDetailedLog(stallsTraceFile_);
    }
    else if (argument == "-callgrind") {
      callgrindTraceFile_ = string(argv[i+1]);
      i++;  // Have used two arguments in this iteration.
      // Wait until all arguments have been parsed before starting the trace.
      // We need to be sure that we know the location of the binary too.
    }
    else if (argument == "-summary") {
      summarise_ = true;
    }
    else if (argument == "-silent") {
      DEBUG = 0;
      silent_ = true;
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
      parameterNames.push_back(parts[0]);
      parameterValues.push_back(parts[1]);
      delete &parts;
    }
    else {
      cerr << "Warning: unrecognised argument: " << argument << endl;
    }
  }

  // There is a default settings file in the config directory.
  if (useDefaultSettings) {
    const string simDir = simulator_.substr(0, simulator_.rfind('/'));
    string settingsFile = simDir + "/../config/loader.txt";
    programFiles.push_back(settingsFile);
  }

  // Perform any setup which must wait until all arguments have been parsed.
  if (!callgrindTraceFile_.empty())
    Callgrind::startTrace(callgrindTraceFile_, programFiles[0]);

}

// Store any relevant arguments in the Loki memory.
// Borrowed from Alex, in turn borrowed from moxie backend.
void Arguments::storeArguments(Chip& chip) {
  vector<Word> data;
  uint i, j, tp;

  /* Target memory looks like this:
     0x00000000 zero word
     0x00000004 argc word
     0x00000008 start of argv
     .
     0x0000???? end of argv
     0x0000???? zero word
     0x0000???? start of data pointed to by argv  */

  data.push_back(Word(0));
  data.push_back(Word(programArgc));

  /* tp is the offset of our first argv data.  */
  tp = 4 + 4 + programArgc*4 + 4;

  /* Set the argv values.  */
  for (i = 0; i < programArgc; i++) {
    data.push_back(Word(tp));
    tp += strlen(programArgv[i]) + 1;
  }

  data.push_back(Word(0));

  uint32_t val = 0;
  int bytes = 0;
  /* Store the strings.  */
  for (i = 0; i < programArgc; i++) {
    for (j=0; j < (strlen(programArgv[i])+1); j++) {
      val |= programArgv[i][j] << (bytes*8);
      bytes++;

      if (bytes == 4) {
        bytes = 0;
        data.push_back(Word(val));
        val = 0;
      }
    }
  }

  if (bytes > 0)
    data.push_back(Word(val));

  chip.storeData(DataBlock(&data, ComponentID(2,0,0), 0, true));
}

void Arguments::setCommandLineParameters() {
  assert(parameterNames.size() == parameterValues.size());
  for (uint i=0; i<parameterNames.size(); i++)
    Parameters::parseParameter(parameterNames[i], parameterValues[i]);
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

bool Arguments::csimTrace()               {return csimTrace_;}
bool Arguments::instructionTrace()        {return instructionTrace_;}
bool Arguments::instructionAddressTrace() {return instructionAddressTrace_;}
bool Arguments::coreTrace()               {return coreTraceFile_.length() > 0;}
bool Arguments::memoryTrace()             {return memTraceFile_.length() > 0;}
bool Arguments::energyTrace()             {return energyTraceFile_.length() > 0;}
bool Arguments::softwareTrace()           {return softwareTraceFile_.length() > 0;}
bool Arguments::lbtTrace()                {return lbtTraceFile_.length() > 0;}
bool Arguments::stallTrace()              {return stallsTraceFile_.length() > 0;}
bool Arguments::callgrindTrace()          {return callgrindTraceFile_.length() > 0;}

bool Arguments::batchMode()               {return batchMode_;}

const string Arguments::invocation()      {return invocation_.str();}

const bool Arguments::summarise()         {return summarise_;}
const bool Arguments::silent()            {return silent_;}

void Arguments::printHelp() {
  cout <<
    "lokisim: a cycle-accurate simulator for the Loki many-core architecture.\n"
    "Usage: lokisim [simulator arguments]\n\n"
    "Options:\n"
    "  -debug\n\tEnter debug mode, where simulator contents can be inspected and changed\n\tat runtime\n"
    "  -trace\n\tPrint each instruction executed and its context to stdout\n"
    "  -run <program>\n\tExecute the supplied program\n"
    "  -summary\n\tPrint a summary of execution behaviour when execution finishes\n"
    "  -silent\n\tPrint nothing except the simulated program's output (and error messages)\n"
    "  -coretrace <file>\n\tDump a trace of instructions executed in a binary format to a file\n"
    "  -memtrace <file>\n\tDump a trace of all memory accesses to a file\n"
    "  -energytrace <file>\n\tDump counts of all significant energy-consuming events to a file\n"
    "  -swtrace <file>\n\tDump snapshots of RF contents, allowing transfer to other simulators\n"
    "  -lbttrace <file>\n\tDump particular types of information to a named file\n"
    "  -stalltrace <file>\n\tDump information about each processor stall to a file\n"
    "  -callgrind <file>\n\tDump output in the Callgrind format\n"
    "  -insttrace\n\tPrint the text form of each instruction executed to stdout\n"
    "  -instaddrtrace\n\tPrint the address of each instruction executed to stdout\n"
    "  -Pparameter=value\n\tSet a named parameter to a particular value\n"
    "  --args [...]\n\tPass all remaining arguments to the simulated program\n"
    "  --help\n\tDisplay this information and exit\n"
    << endl;
}
