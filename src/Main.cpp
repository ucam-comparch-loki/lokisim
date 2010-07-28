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

  string file = "fib.data";
  vector<Word> words = CodeLoader::getData(file);
  vector<Word> insts;

  for(unsigned int i=0; i<words.size(); i+=2) {
    Data d1 = static_cast<Data>(words[i]);
    Data d2 = static_cast<Data>(words[i+1]);

    long l = (long)d1.getData() << 32;
    l += d2.getData();

    Instruction inst(l);
    insts.push_back(inst);
    cout << inst << endl;
  }

  Instruction inst1("setfetchch 52");
  Instruction inst2("fetch r0 0");
  Instruction inst3("ori.eop r0 r0 96 > 51");
  vector<Word> insts2;
  insts2.push_back(inst1); insts2.push_back(inst2); insts2.push_back(inst3);

  tile.storeData(insts, 1);
  tile.storeData(insts, 13);


//  loadFiles();
//
//  CodeLoader::loadCode(tile, directory, coreFiles, memoryFiles);

  int cyclesIdle = 0;

  try {
    for(int i=0; i<20000; i++) {
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
//
//  tile.print(13, 128, 192);
//  tile.print(14, 0, 64);
//  tile.print(15, 0, 64);
//
  Instrumentation::printStats();

  return 0;
}
