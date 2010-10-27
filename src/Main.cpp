/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

//#include <gtest/gtest.h>
#include <systemc.h>
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Debugger.h"
#include "Tests/Test.h"

using std::vector;
using std::string;

//#define GTEST_BREAK_ON_FAILURE 1    // Switch to debug mode if a test fails

int sc_main(int argc, char* argv[]) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();

  Tile tile("tile", 0);
  sc_clock clock("clock", 1, SC_NS, 0.5);
  sc_signal<bool> idle;
  tile.clock(clock);
  tile.idle(idle);

  bool debugMode = false;
  if(argc > 1) {
    string firstArg(argv[1]);
    if(firstArg == "debug") {
      debugMode = true;
      CodeLoader::usingDebugger = true;
    }
  }

  string settingsFile("test_files/loader.txt");
  CodeLoader::loadCode(settingsFile, tile);

  if(debugMode) {
    Debugger::setTile(&tile);
    Debugger::waitForInput();
  }
  else {
    int cyclesIdle = 0;
    int i;

    try {
      for(i=0; i<50000; i++) {
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

  Instrumentation::printStats();

  return 0;
}
