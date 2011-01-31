/*
 * Energy.cpp
 *
 *  Created on: 31 Jan 2011
 *      Author: db434
 */

#include <fstream>
#include "Energy.h"
#include "../InstructionMap.h"
#include "../Parameters.h"
#include "../Statistics.h"
#include "../StringManipulation.h"

bool Energy::libraryLoaded = false;

int regRead=0, regWrite=0, stallReg=0;
int cacheRead=0, cacheWrite=0, tagCheck=0;
int memoryRead=0, memoryWrite=0;
int op=0, multiply=0;
int bitMillimetre=0;

void Energy::loadLibrary(const std::string& filename) {
  std::string fullname = "power_libraries/" + filename;
  std::ifstream file(fullname.c_str());
  char line[100];

  while(file.good()) {
    try {
      file.getline(line, 100);
      string s(line);

      if(s[0]=='%' || s[0]=='\0') continue;   // Skip past any comments

      vector<string>& words = StringManipulation::split(s, ' ');
      if(words.size() < 2) continue;

      // All lines should be of the form "<action> <energy cost>".
      int value = StringManipulation::strToInt(words[1]);

      // This is currently specialised for the ELM power library because I
      // don't know which information we will be able to get in our own one.
      if(words[0] == "add")             op = value;
      else if(words[0] == "mul")        multiply = value;
      else if(words[0] == "regread")    regRead = value;
      else if(words[0] == "regwrite")   regWrite = value;
      else if(words[0] == "stallreg")   stallReg = value;
      else if(words[0] == "tagcheck")   tagCheck = value;
      else if(words[0] == "cacheread")  cacheRead = value;
      else if(words[0] == "cachewrite") cacheWrite = value;
      else if(words[0] == "memread")    memoryRead = value;
      else if(words[0] == "memwrite")   memoryWrite = value;
      else std::cerr << "Warning: can't handle energy costs of " << words[0] << "\n";

      // Rough estimate of communication costs based on "transmitting 1 word
      // 1mm costs about the same as 50 ALU operations".
      if(bitMillimetre == 0) bitMillimetre = op * 50 / 32;

      delete &words;
    }
    catch(std::exception& e) {
      std::cerr << "Error: could not read power library: " << filename << "\n";
      break;
    }
  }

  if(DEBUG) std::cout << "Loaded power library: " << filename << "\n";
  libraryLoaded = true;
}

long Energy::totalEnergy() {
  if(!libraryLoaded) return 0;
  return registerEnergy() + cacheEnergy() + memoryEnergy() +
         operationEnergy() + networkEnergy();
}

double Energy::pJPerOp() {
  return ((double)totalEnergy()/1000) / Statistics::operations();
}

void Energy::printStats() {
  if(!libraryLoaded) return;

  long total   = totalEnergy();
  long regs    = registerEnergy();
  long cache   = cacheEnergy();
  long mem     = memoryEnergy();
  long ops     = operationEnergy();
  long network = networkEnergy();

  cout <<
    "Total energy: " << total << "fJ\t(" << pJPerOp() << "pJ/op)\n" <<
    "  Registers:        " << regs << " (" << asPercentage(regs,total) << ")\n" <<
    "  Cache:            " << cache << " (" << asPercentage(cache,total) << ")\n" <<
    "  Memory:           " << mem << " (" << asPercentage(mem,total) << ")\n" <<
    "  Functional units: " << ops << " (" << asPercentage(ops,total) << ")\n" <<
    "  Network:          " << network << " (" << asPercentage(network,total) << ")\n";
}

long Energy::registerEnergy() {
  return regRead  * Statistics::registerReads() +
         regWrite * Statistics::registerWrites() +
         stallReg * Statistics::stallRegUses();
}

long Energy::cacheEnergy() {
  return cacheRead  * Statistics::cacheReads() +
         cacheWrite * Statistics::cacheWrites() +
         tagCheck   * (Statistics::cacheHits() + Statistics::cacheMisses());
}

long Energy::memoryEnergy() {
  return memoryRead  * Statistics::memoryReads() +
         memoryWrite * Statistics::memoryWrites();
}

long Energy::operationEnergy() {
  // Do we want to include decode energy in here?
  // Note: specialised for the ELM library at the moment. Unsure which
  // information we will eventually have.
  long totalOps   = Statistics::operations();
  long multiplies = Statistics::operations(InstructionMap::MULLW) +
                    Statistics::operations(InstructionMap::MULHW) +
                    Statistics::operations(InstructionMap::MULHWU);
  return multiply * multiplies +
         op       * (totalOps - multiplies);
}

long Energy::networkEnergy() {
  return bitMillimetre * Statistics::networkDistance();
}
