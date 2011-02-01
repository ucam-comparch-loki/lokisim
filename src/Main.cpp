/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include <systemc.h>
#include "Utility/Debugger.h"
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Statistics.h"

using std::vector;
using std::string;

// A small unit of simulation time, allowing signals to propagate
#define TIMESTEP {\
  static int cycleNumber = 0;\
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber++ << " =======" << endl;\
  sc_start(1, SC_NS);\
}

int sc_main(int argc, char* argv[]) {

  Chip chip("chip", 0);
  sc_clock clock("clock", 1, SC_NS, 0.5);
  sc_signal<bool> idle;
  chip.clock(clock);
  chip.idle(idle);

  bool debugMode = false;
  if(argc > 1) {
    string firstArg(argv[1]);
    if(firstArg == "debug") {
      debugMode = true;
      CodeLoader::usingDebugger = true;
    }
    else if(firstArg == "test") {
      // Switch off all status reporting, so we only get the information we
      // want. This allows much faster testing.
      debugMode = true;
      DEBUG = 0;
      Debugger::mode = Debugger::TEST;
    }
  }

  string settingsFile("test_files/loader.txt");
  CodeLoader::loadCode(settingsFile, chip);

  if(debugMode) {
    Debugger::setTile(&chip);
    Debugger::waitForInput();
  }
  else {
    int cyclesIdle = 0;
    int i;

    try {
      for(i=0; i<TIMEOUT; i++) {
        TIMESTEP;
        if(idle.read()) {
          cyclesIdle++;
          if(cyclesIdle >= 10) {
            cout << "\nSystem has been idle for " << cyclesIdle << " cycles." << endl;
            sc_stop();
            Instrumentation::endExecution();
            break;
          }
        }
        else cyclesIdle = 0;
      }
    }
    catch(std::exception& e) {
      // If there's no error message, it might mean that not everything is
      // connected properly.
      cerr << "Execution ended unexpectedly:\n" << e.what() << endl;
    }
  }

  if(DEBUG) Statistics::printStats();

  return 0;
}
