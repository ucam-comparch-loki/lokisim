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

  directory = "dct";
  coreFiles.push_back("0.loki");
  memoryFiles.push_back("12.loki");
  memoryFiles.push_back("weights.data");

//  directory = "zigzag";
//  coreFiles.push_back("0.loki");
//  coreFiles.push_back("1.loki");
//  memoryFiles.push_back("12.loki");
//  memoryFiles.push_back("13.data");

//  directory = ".";
//  coreFiles.push_back("fibonacci_loop.loki");

}

int sc_main(int argc, char* argv[]) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();

  Tile tile("tile", 0);
  sc_clock clock("clock", 1, SC_NS, 0.5);
  sc_signal<bool> idle;
  tile.clock(clock);
  tile.idle(idle);

  string file = "fibonacci/fib.data";
  vector<Word> words = CodeLoader::getData(file);
  vector<Word> insts;

  for(unsigned int i=0; i<words.size(); i+=2) {
    Data d1 = static_cast<Data>(words[i]);
    Data d2 = static_cast<Data>(words[i+1]);

    unsigned long l = (unsigned long)d1.getData() << 32;
    l += d2.getData();

    Instruction inst(l);
    insts.push_back(inst);
    cout << inst << endl;
  }

  string setup = "fibonacci/fib_setup.loki";

  CodeLoader::loadCode(setup, tile, 1);
  tile.storeData(insts, 13);

//  string settingsFile("metaloader.txt");
//  CodeLoader::loadCode(settingsFile, tile);

//  loadFiles();
//
//  CodeLoader::loadCode(tile, directory, coreFiles, memoryFiles);

  int cyclesIdle = 0;
  int i;

  try {
    for(i=0; i<20000; i++) {
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
  catch(std::exception e) {}  // If there's no error message, it might mean
                              // that not everything is connected properly.

//  tile.print(13, 512, 768);
//  tile.print(14, 0, 256);
//  tile.print(15, 0, 256);

  Instrumentation::printStats();

  return 0;
}
