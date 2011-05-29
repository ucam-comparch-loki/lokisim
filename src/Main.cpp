/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include <systemc.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Datatype/AddressedWord.h"
#include "Utility/Debugger.h"
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Statistics.h"

using std::vector;
using std::string;

// Advance the simulation one clock cycle.
static int cycleNumber = 0;

#define TIMESTEP {\
  cycleNumber++;\
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber << " =======" << endl;\
  sc_start(1, SC_NS);\
}

bool debugMode = false;
bool codeLoaded = false;

// Store any relevant arguments in the Loki memory. These arguments are
// separated from the simulator arguments by "--args".
// Borrowed from Alex, in turn borrowed from moxie backend.
void storeArguments(uint argc, char* argv[], Chip& chip) {
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
  chip.writeWord(ComponentID(), 4, Word(argc));

  /* tp is the offset of our first argv data.  */
  tp = 4 + 4 + argc*4 + 4;

  for (i = 0; i < argc; i++)
  {
    /* Set the argv value.  */
    chip.writeWord(ComponentID(), 4 + 4 + i*4, Word(tp));

    /* Store the string.  */
    for (j=0; j < (strlen(argv[i])+1); j++) {
      chip.writeByte(ComponentID(), tp+j, Word(argv[i][j]));
    }
    tp += strlen(argv[i]) + 1;
  }

  chip.writeWord(ComponentID(), 4 + 4 + i*4, Word(0));
}

void parseArguments(uint argc, char* argv[], Chip& chip) {

  if (argc > 1) {
    for(uint i=1; i<argc; i++) {
      string argument(argv[i]);
      if(argument == "debug") {
        // Print out lots of information about execution.
        debugMode = true;
        Debugger::usingDebugger = true;
        Debugger::mode = Debugger::DEBUGGER;
      }
      else if(argument == "test") {
        // Switch off all status reporting, so we only get the information we
        // want. This allows much faster testing.
        debugMode = true;
        DEBUG = 0;
        Debugger::usingDebugger = true;
        Debugger::mode = Debugger::TEST;
      }
      else if(argument == "trace") {
        // Print out only the addresses of each instruction executed.
        DEBUG = 0;
        TRACE = 1;
      }
      else if(argument == "-run") {
        // Command line way of choosing which program to run.
        string filename(argv[i+1]);
        CodeLoader::loadCode(filename, chip);
        i++;  // Have used two arguments in this iteration.
        codeLoaded = true;
        DEBUG = 0;
      }
      else if(argument == "-runbatch") {
        // Command line way of choosing which program to run.
        string filename(argv[i+1]);
        CodeLoader::loadCode(filename, chip);
        i++;  // Have used two arguments in this iteration.
        codeLoaded = true;
        DEBUG = 0;
        BATCH_MODE = 1;
      }
      else if(argument == "--args") {
        // Pass command line options to the simulated program.
        argv[i] = argv[0];
        storeArguments(argc-i, argv+i, chip);
      }
    }
  }

}

void simulate(Chip& chip) {

  sc_clock clock("clock", 1, SC_NS, 0.5);
  sc_signal<bool> idle;
  chip.clock(clock);
  chip.idle(idle);

  if(debugMode) {
    Debugger::setChip(&chip);
    Debugger::waitForInput();
  }
  else {
    int cyclesIdle = 0;

    try {
      int i;
      for(i=0; i<TIMEOUT && !sc_core::sc_end_of_simulation_invoked(); i++) {
        TIMESTEP;
        if(idle.read()) {
          cyclesIdle++;
          if(cyclesIdle >= 100) {
            sc_stop();
            Instrumentation::endExecution();
            cerr << "\nSystem has been idle for " << cyclesIdle << " cycles." << endl;
            RETURN_CODE = EXIT_FAILURE;
            break;
          }
        }
        else cyclesIdle = 0;
      }

      if(i >= TIMEOUT) {
        cerr << "Simulation timed out after " << TIMEOUT << " cycles." << endl;
        RETURN_CODE = EXIT_FAILURE;
      }
    }
    catch(std::exception& e) {
      // If there's no error message, it might mean that not everything is
      // connected properly.
      cerr << "Execution ended unexpectedly:\n" << e.what() << endl;
      RETURN_CODE = EXIT_FAILURE;
    }
  }

}

int sc_main(int argc, char* argv[]) {
	// Override parameters before instantiating chip model

	bool batchMode = false;

	if (argc >= 3) {
		string firstArg(argv[1]);
		if (firstArg == "-runbatch")
			batchMode = true;
	}

	string settingsFile(batchMode ? argv[2] : "test_files/loader.txt");
	CodeLoader::loadParameters(settingsFile);

	// Instantiate chip model - parameters must not be changed after this or undefined behaviour might occur

	Chip chip("chip", 0);

	// Load code to execute

	parseArguments(argc, argv, chip);

	if (!codeLoaded)
		CodeLoader::loadCode(settingsFile, chip);

	CodeLoader::makeExecutable(chip);

	// Run simulation

	simulate(chip);

	// Print debug information

	if (DEBUG || BATCH_MODE) {
		Parameters::printParameters();
		Statistics::printStats();
	}

	return RETURN_CODE;
}
