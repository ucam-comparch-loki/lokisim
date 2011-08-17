/*
 * Debugger.cpp
 *
 *  Created on: 6 Aug 2010
 *      Author: db434
 */

#include "Debugger.h"
#include "Statistics.h"
#include "StringManipulation.h"
#include "../Chip.h"
#include "../Datatype/DecodedInst.h"
#include "../Datatype/Instruction.h"

bool Debugger::usingDebugger = false;
int Debugger::mode = NOT_IN_USE;

bool Debugger::hitBreakpoint = false;
Chip* Debugger::chip = 0;
vector<MemoryAddr> Debugger::breakpoints;

int Debugger::cycleNumber = 0;
ComponentID Debugger::defaultCore(0, 0);
ComponentID Debugger::defaultInstMemory(0, CORES_PER_TILE);
ComponentID Debugger::defaultDataMemory(0, CORES_PER_TILE + 1);

int Debugger::cyclesIdle = 0;
int Debugger::maxIdleTime = 10;

// _S = short version
const string BREAKPOINT   = "breakpoint";
const string BREAKPOINT_S = "bp";
const string CONTINUE     = "continue";
const string CONTINUE_S   = "c";
const string EXECUTE      = "execute";
const string EXECUTE_S    = "e";
const string FINISH       = "finish";
const string FINISH_S     = "f";
const string HELP         = "help";
const string HELP_S       = "h";
const string PRINTREGS    = "printregs";
const string PRINTREGS_S  = "pr";
const string PRINTPRED    = "printpred";
const string PRINTPRED_S  = "pp";
const string PRINTSTACK   = "printstack";
const string PRINTSTACK_S = "ps";
const string MEMORY       = "printmem";
const string MEMORY_S     = "pm";
const string VERBOSE      = "verbose";
const string VERBOSE_S    = "v";
const string CHANGECORE   = "changecore";
const string CHANGECORE_S = "cc";
const string CHANGEMEM    = "changememory";
const string CHANGEMEM_S  = "cm";
const string STATISTIC    = "statistic";
const string STATISTIC_S  = "s";
const string QUIT         = "quit";
const string QUIT_S       = "q";


void Debugger::waitForInput() {
  if(mode == DEBUGGER)
    cout << "\nEntering debug mode. Press return to begin execution." << endl;

  while(!sc_core::sc_end_of_simulation_invoked()) {
    string input;
    std::getline(std::cin, input);
    vector<string>& words = StringManipulation::split(input, ' ');

    if(input == "") {
      executeSingleCycle();
    }
    else if(words[0] == BREAKPOINT || words[0] == BREAKPOINT_S) {
      words.erase(words.begin());
      setBreakPoint(parseIntVector(words, false));
    }
    else if(words[0] == CONTINUE   || words[0] == CONTINUE_S) {
      executeUntilBreakpoint();
    }
    else if(words[0] == EXECUTE    || words[0] == EXECUTE_S) {
      // The instruction has been split up, so join it back together again.
      string inst = "";
      for(uint i=1; i<words.size(); i++) inst += (words[i] + " ");

      execute(inst);
    }
    else if(words[0] == FINISH     || words[0] == FINISH_S) {
      finishExecution();
    }
    else if(words[0] == HELP       || words[0] == HELP_S) {
      printHelp();
    }
    else if(words[0] == PRINTREGS  || words[0] == PRINTREGS_S) {
      words.erase(words.begin());
      printRegs(parseIntVector(words, true));
    }
    else if(words[0] == PRINTPRED  || words[0] == PRINTPRED_S) {
      printPred();
    }
    else if(words[0] == PRINTSTACK || words[0] == PRINTSTACK_S) {
      printStack();
    }
    else if(words[0] == MEMORY     || words[0] == MEMORY_S) {
      words.erase(words.begin());
      printMemLocations(parseIntVector(words, false));
    }
    else if(words[0] == CHANGECORE || words[0] == CHANGECORE_S) {
      if(words.size() > 1) {
        // Warning: assuming tile 0.
        ComponentID id(0, StringManipulation::strToInt(words[1]));
        changeCore(id);
      }
    }
    else if(words[0] == CHANGEMEM  || words[0] == CHANGEMEM_S) {
      if(words.size() > 1) {
        // Warning: assuming tile 0.
        ComponentID id(0, StringManipulation::strToInt(words[1]));
        changeMemory(id);
      }
    }
    else if(words[0] == VERBOSE    || words[0] == VERBOSE_S) {
      DEBUG = !DEBUG;
      if(mode == DEBUGGER)
        cout << "Switching to " << (DEBUG?"high":"low") << "-detail mode." << endl;
    }
    else if(words[0] == STATISTIC  || words[0] == STATISTIC_S) {
      int parameter = -1;
      if(words.size() > 2) parameter = StringManipulation::strToInt(words[2]);
      printStatistic(words[1], parameter);
    }
    else if(words[0] == QUIT       || words[0] == QUIT_S || words[0] == "exit") {
      Instrumentation::endExecution();
      break;
    }
    else {
      cout << "Unrecognised command: " << words[0] << "\n";
      cout << "Type \"help\" for a list of commands.\n";
    }

    delete &words;
  }
}

void Debugger::setBreakPoint(vector<int>& bps, const ComponentID& memory) {

  for(uint j=0; j<bps.size(); j++) {
    MemoryAddr addr = bps[j];

    if(isBreakpoint(addr)) {
      if(mode == DEBUGGER)
        cout << "Removed breakpoint: " << chip->readWord(memory, addr) << endl;
      removeBreakpoint(addr);
    }
    else {
      if(mode == DEBUGGER)
        cout << "Set breakpoint: " << chip->readWord(memory, addr) << endl;
      addBreakpoint(addr);
    }
  }

}

void Debugger::printStack(const ComponentID& core, const ComponentID& memory) {
  static const RegisterIndex stackReg = 2;
  static const int frameSize = 28;

  int stackPointer = chip->readRegister(core, stackReg);
  chip->print(memory, stackPointer, stackPointer+frameSize);
}

void Debugger::printMemLocations(vector<int>& locs, const ComponentID& memory) {
  if(mode == DEBUGGER)
    cout << "Printing value(s) from memory " << memory << endl;

  for(uint i=0; i<locs.size(); i++) {
    int val = chip->readWord(memory, locs[i]).toInt();

    if(mode == DEBUGGER) cout << locs[i] << "\t";
    cout << val << "\n";
  }
}

void Debugger::printRegs(vector<int>& regs, const ComponentID& core) {
  if(mode == DEBUGGER)
    cout << "Printing core " << core << "'s register values:" << endl;

  if(regs.size() == 0) {
    // If no registers were specified, print them all.
    for(uint i=0; i<NUM_ADDRESSABLE_REGISTERS; i++) {  // physical regs instead?
      int regVal = chip->readRegister(core, i);

      if(mode == DEBUGGER) cout << "r" << i << "\t";
      cout << regVal << "\n";
    }
  }
  else {
    for(uint i=0; i<regs.size(); i++) {
      int regVal = chip->readRegister(core, regs[i]);

      if(mode == DEBUGGER) cout << "r" << regs[i] << "\t";
      cout << regVal << "\n";
    }
  }
}

void Debugger::printPred(const ComponentID& core) {
  if(mode == DEBUGGER) cout << "pred\t";
  cout << chip->readPredicate(core) << "\n";
}

void Debugger::executedInstruction(DecodedInst inst, const ComponentID& core, bool executed) {

  if(mode == DEBUGGER) {
    cout << core << ":\t" << "[" << inst.location() << "]\t" << inst
         << (executed ? "" : " (not executed)") << endl;
  }

  if(isBreakpoint(inst.location())) {
    hitBreakpoint = true;
    defaultCore = core;
//    defaultInstMemory = inst.location().channelID();
  }

}

void Debugger::setChip(Chip* c) {
  chip = c;
}

void Debugger::executeSingleCycle() {
  executeNCycles(1);
}

void Debugger::executeNCycles(int n) {
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber << " =======" << "\n";
  sc_start(n, sc_core::SC_NS);

  cycleNumber += n;

  // We can't assume that the chip has been idle for all n cycles, so only
  // increment by 1. The idle count still works though - it is unlikely that
  // the chip will be idle on many successive probes of the idle value.
  if(chip->isIdle()) cyclesIdle++;
  else cyclesIdle = 0;
}

void Debugger::executeUntilBreakpoint() {
  if(breakpoints.empty()) {
    executeSingleCycle();
  }
  else {
    while(!hitBreakpoint && cyclesIdle<maxIdleTime && cycleNumber<TIMEOUT &&
          !sc_core::sc_end_of_simulation_invoked()) {
      executeSingleCycle();
    }
  }

  if(cycleNumber >= TIMEOUT) {
    cerr << "Simulation timed out after " << TIMEOUT << " cycles." << endl;
    RETURN_CODE = 1;
  }
  else if(cyclesIdle >= maxIdleTime) {
    cerr << "System was idle for " << cyclesIdle << " cycles." << endl;
    RETURN_CODE = 1;
  }
  else if(mode == DEBUGGER) cout << "Now at cycle " << cycleNumber << "\n";

  hitBreakpoint = false;
}

void Debugger::finishExecution() {
  while(cyclesIdle<maxIdleTime && cycleNumber<TIMEOUT &&
        !sc_core::sc_end_of_simulation_invoked()) {
    executeNCycles(100);
  }

  if(cycleNumber >= TIMEOUT) {
    cerr << "Simulation timed out after " << TIMEOUT << " cycles." << endl;
    RETURN_CODE = 1;
  }
  else if(cyclesIdle >= maxIdleTime) {
    cerr << "System was idle for " << cyclesIdle << " cycles." << endl;
    RETURN_CODE = 1;
  }
  else if(mode == DEBUGGER) cout << "\nExecution ended successfully.\n";

//  Instrumentation::endExecution();
}

void Debugger::execute(string instruction) {
  // Create an instruction and pass it to a core.
  Instruction inst(instruction);
  vector<Word> instVector;

  if(BYTES_PER_INSTRUCTION > BYTES_PER_WORD) {
    // Need to split the instruction into multiple words.
    uint64_t val = (uint64_t)inst.toLong();
    Word first(val >> 32);
    Word second(val & 0xFFFFFFFF);
    instVector.push_back(first);
    instVector.push_back(second);
  }
  else instVector.push_back(inst);

  if(mode == DEBUGGER)
    cout << "Passing " << inst << " to core " << defaultCore << "\n";
  chip->storeInstructions(instVector, defaultCore);

  cyclesIdle = 0;
}

void Debugger::changeCore(const ComponentID& core) {
  // Should this be a persistent change? Currently, it will change as soon as
  // a different core executes a breakpoint instruction.
  if(mode == DEBUGGER)
    cout << "Will print details for core " << core
         << " until next breakpoint is reached.\n";

  defaultCore = core;
}

void Debugger::changeMemory(const ComponentID& memory) {
  if(mode == DEBUGGER)
    cout << "Will now print details for memory " << memory << "\n";

  defaultDataMemory = memory;
}

vector<int>& Debugger::parseIntVector(vector<string>& words, bool registers) {
  vector<int>* locations = new vector<int>();

  for(uint i=0; i<words.size(); i++) {
    string location = words[i];

    // "+XX" tells us to use the XX values immediately following the previous
    // value (and including the previous value).
    // Example: "0 +64" gives 64 consecutive values, starting at address 0.
    if(location[0] == '+') {
      location.erase(0,1);                           // Remove "+" from "+XX"
      int num = StringManipulation::strToInt(location);
      int start = locations->back();

      for(int j=1; j<num; j++) {
        if(registers) locations->push_back(start+j);
        else          locations->push_back(start + j*BYTES_PER_WORD);
      }

      continue;
    }
    else if(location[0] == 'r') location.erase(0,1); // Remove "r" from "rXX"

    locations->push_back(StringManipulation::strToInt(location));
  }

  return *locations;
}

bool Debugger::isBreakpoint(MemoryAddr addr) {
  for(uint j=0; j<breakpoints.size(); j++) {
    if(addr==breakpoints[j]) return true;
  }
  return false;
}

void Debugger::addBreakpoint(MemoryAddr addr) {
  breakpoints.push_back(addr);
}

void Debugger::removeBreakpoint(MemoryAddr addr) {
  for(uint j=0; j<breakpoints.size(); j++) {
    if(addr==breakpoints[j]) breakpoints.erase(breakpoints.begin()+j);
  }
}

void Debugger::printStatistic(const std::string& statName, int parameter) {
  cout << Statistics::getStat(statName, parameter) << "\n";
}

// Formats the list of commands
#define COMMAND(name,description) "  " << name << " (" << name ## _S << ")\n" <<\
  "    " << description

void Debugger::printHelp() {

  cout << "Available commands:" << endl <<
          "  [no input]" << endl <<
          "    Execute a single cycle" << endl <<
    COMMAND(BREAKPOINT,
      "Toggle a breakpoint at a particular instruction. Specify the" <<
      " instruction(s)\n    using its index in memory") << endl <<
    COMMAND(CONTINUE,
      "Continue execution until a breakpoint is reached") << endl <<
    COMMAND(FINISH,
      "Continue execution until the program completes or an error" <<
      " is thrown") << endl <<
    COMMAND(QUIT,
      "Terminate execution and show overall statistics") << endl <<
    COMMAND(PRINTREGS,
      "Print the current values of all specified registers, or all" <<
      " registers if\n    none were selected") << endl <<
    COMMAND(PRINTPRED,
      "Print the current value of the predicate register") << endl <<
    COMMAND(PRINTSTACK,
      "Print the contents of the current stack frame") << endl <<
    COMMAND(MEMORY,
      "Print the contents of arbitrary locations in memory") << endl <<
    COMMAND(CHANGECORE,
      "Change the core which data is printed for") << endl <<
    COMMAND(CHANGEMEM,
      "Change the memory which data is printed for") << endl <<
    COMMAND(VERBOSE,
      "Switch between high and low detail information") << endl;

}

#undef COMMAND
