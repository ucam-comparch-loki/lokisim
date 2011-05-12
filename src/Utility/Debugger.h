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
#include "../Datatype/ComponentID.h"
#include "../Typedefs.h"

using std::string;
using std::vector;

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
  static void executedInstruction(DecodedInst i, const ComponentID& core, bool executed);

  // Choose the tile to debug. Make this more general later.
  static void setChip(Chip* c);

private:

  // Set a breakpoint corresponding to an instruction location.
  static void setBreakPoint(vector<int>& bps, const ComponentID& memory=defaultInstMemory);
  static bool isBreakpoint(MemoryAddr addr);
  static void addBreakpoint(MemoryAddr addr);
  static void removeBreakpoint(MemoryAddr addr);

  // Print the current stack.
  static void printStack(const ComponentID& core=defaultCore,
		                 const ComponentID& memory=defaultDataMemory);

  static void printMemLocations(vector<int>& locs, const ComponentID& memory=defaultDataMemory);

  // Print the values of the given registers.
  static void printRegs(vector<int>& regs, const ComponentID& core=defaultCore);

  // Print the value of the predicate register.
  static void printPred(const ComponentID& core=defaultCore);

  // Print the named statistic, using the parameter to, for example, choose a
  // core or an ALU operation.
  static void printStatistic(const std::string& statName, int parameter=-1);

  // Print a list of possible commands.
  static void printHelp();

  static void executeSingleCycle();
  static void executeUntilBreakpoint();
  static void finishExecution();

  static void execute(string instruction);

  static void changeCore(const ComponentID& core);
  static void changeMemory(const ComponentID& memory);

  static vector<int>& parseIntVector(vector<string>& s, bool registers);

//==============================//
// Local state
//==============================//

public:

  static int mode;
  static bool usingDebugger;

  // Test mode uses much of the same functionality as debug mode, but only
  // wants the bare minimum of information printed out.
  enum Mode {NOT_IN_USE, DEBUGGER, TEST};

private:

  static bool hitBreakpoint;

  static vector<MemoryAddr> breakpoints;
  static Chip* chip;

  static int cycleNumber;
  static ComponentID defaultCore;
  static ComponentID defaultInstMemory;
  static ComponentID defaultDataMemory;

  static int cyclesIdle;
  static int maxIdleTime;

};

#endif /* DEBUGGER_H_ */
