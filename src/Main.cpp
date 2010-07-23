/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include <systemc.h>
#include "Utility/CodeLoader.h"
#include "Tests/Test.h"

using std::vector;
using std::string;

#define GTEST_BREAK_ON_FAILURE 1    // Switch to debug mode if a test fails

string directory;
vector<string> coreFiles;
vector<string> memoryFiles;

void loadFiles() {

//  directory = "dct";
//  coreFiles.push_back("0.loki");
//  memoryFiles.push_back("12.loki");
//  memoryFiles.push_back("weights.data");

  directory = "zigzag";
  coreFiles.push_back("0.loki");
  coreFiles.push_back("1.loki");
  memoryFiles.push_back("12.loki");
  memoryFiles.push_back("13.data");

}

int sc_main(int argc, char* argv[]) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();

  Tile tile("tile", 0);
  sc_clock clock("clock", 1, SC_NS, 0.5);
  sc_signal<bool> idle;
  tile.clock(clock);
  tile.idle(idle);

  loadFiles();

  CodeLoader::loadCode(tile, directory, coreFiles, memoryFiles);

  int cyclesIdle = 0;

  try {
    for(int i=0; i<100000; i++) {
      TIMESTEP;
      if(idle.read()) {
        cyclesIdle++;
        if(cyclesIdle >= 5) {
          sc_stop();
          break;
        }
      }
      else cyclesIdle = 0;
    }
  }
  catch(std::exception e) {}

  Instrumentation::printStats();

  return 0;
}
