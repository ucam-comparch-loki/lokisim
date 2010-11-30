/*
 * Debugger.h
 *
 *  Created on: 6 Aug 2010
 *      Author: db434
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <vector>
#include <string>
#include "../Typedefs.h"

using std::string;
using std::vector;

class Address;
class DecodedInst;
class Chip;

class Debugger {

//==============================//
// Methods
//==============================//

public:

  // Wait for the user to issue a command.
  static void waitForInput();

  // Receive notification that a particular core executed a particular
  // instruction.
  static void executedInstruction(DecodedInst i, ComponentID core, bool executed);

  // Choose the tile to debug. Make this more general later.
  static void setTile(Chip* c);

private:

  // Set a breakpoint corresponding to an instruction location.
  static void setBreakPoint(vector<int>& bps, ComponentID memory=defaultInstMemory);
  static bool isBreakpoint(Address addr);
  static void addBreakpoint(Address addr);
  static void removeBreakpoint(Address addr);

  // Print the current stack.
  static void printStack(ComponentID core=defaultCore,
                         ComponentID memory=defaultDataMemory);

  static void printMemLocations(vector<int>& locs, ComponentID memory=defaultDataMemory);

  // Print the values of the given registers.
  static void printRegs(vector<int>& regs, ComponentID core=defaultCore);

  // Print the value of the predicate register.
  static void printPred(ComponentID core=defaultCore);

  // Print a list of possible commands.
  static void printHelp();

  static void executeSingleCycle();
  static void executeUntilBreakpoint();
  static void finishExecution();

  static void changeCore(ComponentID core);
  static void changeMemory(ComponentID memory);

  static vector<int>& parseIntVector(vector<string>& s);

//==============================//
// Local state
//==============================//

private:

  static bool hitBreakpoint;

  static vector<Address> breakpoints;
  static Chip* chip;

  static int cycleNumber;
  static ComponentID defaultCore;
  static ComponentID defaultInstMemory;
  static ComponentID defaultDataMemory;

  static int cyclesIdle;
  static int maxIdleTime;

};

#endif /* DEBUGGER_H_ */
