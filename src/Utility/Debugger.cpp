/*
 * Debugger.cpp
 *
 *  Created on: 6 Aug 2010
 *      Author: db434
 */

#include "Debugger.h"
#include "StringManipulation.h"
#include "../Chip.h"
#include "../Datatype/DecodedInst.h"
#include "../Datatype/Instruction.h"

bool Debugger::hitBreakpoint = false;
Chip* Debugger::chip = 0;
vector<Address> Debugger::breakpoints;

int Debugger::cycleNumber = 0;
ComponentID Debugger::defaultCore = 1;
ComponentID Debugger::defaultInstMemory = 12;
ComponentID Debugger::defaultDataMemory = 13;

int Debugger::cyclesIdle = 0;
int Debugger::maxIdleTime = 10;

// _S = short version
const string BREAKPOINT   = "breakpoint";
const string BREAKPOINT_S = "bp";
const string CONTINUE     = "continue";
const string CONTINUE_S   = "c";
const string FINISH       = "finish";
const string FINISH_S     = "f";
const string PRINTREGS    = "printregs";
const string PRINTREGS_S  = "pr";
const string PRINTPRED    = "printpred";
const string PRINTPRED_S  = "pp";
const string PRINTSTACK   = "printstack";
const string PRINTSTACK_S = "ps";
const string MEMORY       = "memory";
const string MEMORY_S     = "m";
const string VERBOSE      = "verbose";
const string VERBOSE_S    = "v";
const string CHANGECORE   = "changecore";
const string CHANGECORE_S = "cc";
const string CHANGEMEM    = "changememory";
const string CHANGEMEM_S  = "cm";
const string QUIT         = "quit";
const string QUIT_S       = "q";


void Debugger::waitForInput() {
  cout << "\nEntering debug mode. Press return to begin execution." << endl;

  while(cyclesIdle < maxIdleTime && cycleNumber < TIMEOUT) {
    string input;
    std::getline(std::cin, input);
    vector<string>& words = StringManipulation::split(input, ' ');

    if(input == "") {
      executeSingleCycle();
    }
    else if(words[0] == BREAKPOINT || words[0] == BREAKPOINT_S) {
      words.erase(words.begin());
      setBreakPoint(parseIntVector(words));
    }
    else if(words[0] == CONTINUE   || words[0] == CONTINUE_S) {
      executeUntilBreakpoint();
    }
    else if(words[0] == FINISH     || words[0] == FINISH_S) {
      finishExecution();
    }
    else if(words[0] == PRINTREGS  || words[0] == PRINTREGS_S) {
      words.erase(words.begin());
      printRegs(parseIntVector(words));
    }
    else if(words[0] == PRINTPRED  || words[0] == PRINTPRED_S) {
      printPred();
    }
    else if(words[0] == PRINTSTACK || words[0] == PRINTSTACK_S) {
      printStack();
    }
    else if(words[0] == MEMORY     || words[0] == MEMORY_S) {
      words.erase(words.begin());
      printMemLocations(parseIntVector(words));
    }
    else if(words[0] == CHANGECORE || words[0] == CHANGECORE_S) {
      if(words.size() > 1)
        changeCore(StringManipulation::strToInt(words[1]));
    }
    else if(words[0] == CHANGEMEM  || words[0] == CHANGEMEM_S) {
      if(words.size() > 1)
        changeMemory(StringManipulation::strToInt(words[1]));
    }
    else if(words[0] == VERBOSE    || words[0] == VERBOSE_S) {
      DEBUG = !DEBUG;
      cout << "Switching to " << (DEBUG?"high":"low") << "-detail mode." << endl;
    }
    else if(words[0] == QUIT       || words[0] == QUIT_S || words[0] == "exit") {
      Instrumentation::endExecution();
      break;
    }
    else {
      printHelp();
    }

    delete &words;
  }
}

void Debugger::setBreakPoint(vector<int>& bps, ComponentID memory) {

  for(uint j=0; j<bps.size(); j++) {
    Address addr(bps[j], memory);

    if(isBreakpoint(addr)) {
      cout << "Removed breakpoint: " << chip->getMemVal(memory, bps[j]) << endl;
      removeBreakpoint(addr);
    }
    else {
      cout << "Set breakpoint: " << chip->getMemVal(memory, bps[j]) << endl;
      addBreakpoint(addr);
    }
  }

}

void Debugger::printStack(ComponentID core, ComponentID memory) {
  static const RegisterIndex stackReg = 2;
  static const int frameSize = 28;

  int stackPointer = chip->getRegVal(core, stackReg);
  chip->print(memory, stackPointer, stackPointer+frameSize);
}

void Debugger::printMemLocations(vector<int>& locs, ComponentID memory) {
  cout << "Printing value(s) from memory " << memory << endl;
  for(uint i=0; i<locs.size(); i++) {
    int val = chip->getMemVal(memory, locs[i]).toInt();
    cout << locs[i] << "\t" << val << endl;
  }
}

void Debugger::printRegs(vector<int>& regs, ComponentID core) {
  cout << "Printing core " << core << "'s register values:" << endl;
  if(regs.size() == 0) {
    // If no registers were specified, print them all.
    for(uint i=0; i<NUM_ADDRESSABLE_REGISTERS; i++) {  // physical regs instead?
      int regVal = chip->getRegVal(core, i);
      cout << "r" << i << "\t" << regVal << endl;
    }
  }
  else {
    for(uint i=0; i<regs.size(); i++) {
      int regVal = chip->getRegVal(core, regs[i]);
      cout << "r" << regs[i] << "\t" << regVal << endl;
    }
  }
}

void Debugger::printPred(ComponentID core) {
  cout << "pred\t" << chip->getPredReg(core) << endl;
}

void Debugger::executedInstruction(DecodedInst inst, ComponentID core, bool executed) {

  cout << core << ":\t" << "[" << inst.location().address() << "]\t" << inst
       << (executed ? "" : " (not executed)") << endl;

  if(isBreakpoint(inst.location())) {
    hitBreakpoint = true;
    defaultCore = core;
    defaultInstMemory = inst.location().channelID();
  }

}

void Debugger::setTile(Chip* c) {
  chip = c;
}

void Debugger::executeSingleCycle() {
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber << " =======" << endl;
  sc_start(1, sc_core::SC_NS);

  cycleNumber++;

  if(chip->isIdle()) cyclesIdle++;
  else cyclesIdle = 0;
}

void Debugger::executeUntilBreakpoint() {
  if(breakpoints.empty()) {
    executeSingleCycle();
  }
  else {
    while(!hitBreakpoint && cyclesIdle<maxIdleTime && cycleNumber<TIMEOUT) {
      executeSingleCycle();
    }
  }

  hitBreakpoint = false;
}

void Debugger::finishExecution() {
  while(cyclesIdle<maxIdleTime && cycleNumber<TIMEOUT) executeSingleCycle();
}

void Debugger::changeCore(ComponentID core) {
  // Should this be a persistent change? Currently, it will change as soon as
  // a different core executes a breakpoint instruction.
  cout << "Will now print details for core " << core <<
          " until next breakpoint is reached." << endl;

  defaultCore = core;
}

void Debugger::changeMemory(ComponentID memory) {
  cout << "Will now print details for memory " << memory << endl;

  defaultDataMemory = memory;
}

vector<int>& Debugger::parseIntVector(vector<string>& words) {
  vector<int>* regs = new vector<int>();

  for(uint i=0; i<words.size(); i++) {
    string reg = words[i];
    if(reg[0] == 'r') reg.erase(0,1);
    regs->push_back(StringManipulation::strToInt(reg));
  }

  return *regs;
}

bool Debugger::isBreakpoint(Address addr) {
  for(uint j=0; j<breakpoints.size(); j++) {
    if(addr==breakpoints[j]) return true;
  }
  return false;
}

void Debugger::addBreakpoint(Address addr) {
  breakpoints.push_back(addr);
}

void Debugger::removeBreakpoint(Address addr) {
  for(uint j=0; j<breakpoints.size(); j++) {
    if(addr==breakpoints[j]) breakpoints.erase(breakpoints.begin()+j);
  }
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
