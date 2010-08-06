/*
 * Debugger.cpp
 *
 *  Created on: 6 Aug 2010
 *      Author: db434
 */

#include "Debugger.h"
#include "StringManipulation.h"

bool Debugger::hitBreakpoint = false;
Tile* Debugger::tile = 0;
vector<Instruction> Debugger::breakpoints;

int Debugger::cycleNumber = 0;
int Debugger::defaultCore = 1;
int Debugger::defaultInstMemory = 13;
int Debugger::defaultDataMemory = 12;

int Debugger::cyclesIdle = 0;

void Debugger::waitForInput() {
  cout << "\nEntering debug mode. Press return to begin execution." << endl;

  while(cyclesIdle < 5) {
    string input;
    std::getline(std::cin, input);
    vector<string> words = StringManipulation::split(input, ' ');

    if(tile->isIdle()) cyclesIdle++;
    else cyclesIdle = 0;

    if(input == "") {
      executeUntilBreakpoint();
    }
    else if(words[0] == "breakpoint" || words[0] == "bp") {
      int inst = StringManipulation::strToInt(words[1]);
      setBreakPoint(inst);
    }
    else if(words[0] == "printregs" || words[0] == "pr") {
      words.erase(words.begin());
      printRegs(parseRegs(words));
    }
    else if(words[0] == "printstack" || words[0] == "ps") {
      printStack();
    }
    else if(words[0] == "verbose" || words[0] == "v") {
      // DEBUG = !DEBUG;
    }
    else if(words[0] == "quit" || words[0] == "q" || words[0] == "exit") {
      Instrumentation::endExecution();
      break;
    }
  }
}

void Debugger::setBreakPoint(int bp, int memory) {
  Word w = tile->getMemVal(memory, bp);
  Instruction i = static_cast<Instruction>(w);

  cout << "Set breakpoint: " << i << endl;

  breakpoints.push_back(i);
}

void Debugger::printStack(int core, int memory) {
  static const int stackReg = 2;
  static const int frameSize = 28;

  int stackPointer = tile->getRegVal(core, stackReg);
  tile->print(memory, stackPointer, stackPointer+frameSize);
}

void Debugger::printRegs(vector<int> regs, int core) {
  for(unsigned int i=0; i<regs.size(); i++) {
    int regVal = tile->getRegVal(core, regs[i]);
    cout << "r" << regs[i] << "\t" << regVal << endl;
  }
}

void Debugger::executedInstruction(Instruction inst, int core) {

  cout << core << "\t" << inst << endl;

  for(unsigned int i=0; i<breakpoints.size(); i++) {
    if(breakpoints[i] == inst) {
      hitBreakpoint = true;

      defaultCore = core;
      // Could also find out what its instruction memory is using its fetch ch?

      break;
    }
  }

}

void Debugger::setTile(Tile* t) {
  tile = t;
}

void Debugger::executeSingleCycle() {
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber++ << " =======" << endl;
  sc_start(1, sc_core::SC_NS);
}

void Debugger::executeUntilBreakpoint() {
  if(breakpoints.empty()) {
    executeSingleCycle();
  }
  else {
    while(!hitBreakpoint && cyclesIdle<5) {
      executeSingleCycle();
    }
  }

  hitBreakpoint = false;
}

vector<int>& Debugger::parseRegs(vector<string>& words) {
  vector<int>* regs = new vector<int>();

  for(unsigned int i=0; i<words.size(); i++) {
    string reg = words[i];
    if(reg[0] == 'r') reg.erase(0,1);
    regs->push_back(StringManipulation::strToInt(reg));
  }

  return *regs;
}
