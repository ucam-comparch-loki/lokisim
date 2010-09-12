/*
 * Debugger.h
 *
 *  Created on: 6 Aug 2010
 *      Author: db434
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <vector>
#include "../Tile.h"
#include "../Datatype/Instruction.h"

using std::string;

class Debugger {

//==============================//
// Methods
//==============================//

public:

  // Wait for the user to issue a command.
  static void waitForInput();

  // Receive notification that a particular core executed a particular
  // instruction.
  static void executedInstruction(Instruction i, int core);

  // Choose the tile to debug. Make this more general later.
  static void setTile(Tile* t);

private:

  // Set a breakpoint corresponding to an instruction location.
  static void setBreakPoint(vector<int> bps, int memory=defaultInstMemory);
  static bool isBreakpoint(Instruction i);
  static void addBreakpoint(Instruction i);
  static void removeBreakpoint(Instruction i);

  // Print the current stack.
  static void printStack(int core=defaultCore, int memory=defaultDataMemory);

  static void printMemLocations(vector<int> locs, int memory=defaultDataMemory);

  // Print the values of the given registers.
  static void printRegs(vector<int> regs, int core=defaultCore);

  // Print the value of the predicate register.
  static void printPred(int core=defaultCore);

  // Print a list of possible commands.
  static void printHelp();

  static void executeSingleCycle();
  static void executeUntilBreakpoint();
  static void finishExecution();

  static void changeCore(int core);
  static void changeMemory(int memory);

  static vector<int>& parseIntVector(vector<string>& s);

//==============================//
// Local state
//==============================//

private:

  static bool hitBreakpoint;

  static vector<Instruction> breakpoints;
  static Tile* tile;

  static int cycleNumber;
  static int defaultCore;
  static int defaultInstMemory;
  static int defaultDataMemory;

  static int cyclesIdle;

};

#endif /* DEBUGGER_H_ */