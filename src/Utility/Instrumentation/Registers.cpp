/*
 * Registers.cpp
 *
 *  Created on: 21 Jan 2011
 *      Author: db434
 */

#include "../../Datatype/ComponentID.h"
#include "Registers.h"

unsigned long long Registers::numReads_ = 0;
unsigned long long Registers::numWrites_ = 0;
unsigned long long Registers::numForwards_ = 0;
unsigned long long Registers::stallRegs_ = 0;

// TODO: determine which reads/writes were wasted, and could have been replaced
// by explicit data forwarding.
// This is the case when a value is forwarded, and then that register is written
// before it is read (or not read at all).

void Registers::read(const ComponentID& core, RegisterIndex reg)    {numReads_++;}
void Registers::write(const ComponentID& core, RegisterIndex reg)   {numWrites_++;}
void Registers::forward(const ComponentID& core, RegisterIndex reg) {numForwards_++;}
void Registers::stallReg(const ComponentID& core)                   {stallRegs_++;}

unsigned long long  Registers::numReads()      {return numReads_;}
unsigned long long  Registers::numWrites()     {return numWrites_;}
unsigned long long  Registers::numForwards()   {return numForwards_;}
unsigned long long  Registers::stallRegUses()  {return stallRegs_;}

void Registers::printStats() {
  if (BATCH_MODE) {
	cout << "<@GLOBAL>regs_reads:" << numReads() << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>regs_writes:" << numWrites() << "</@GLOBAL>" << endl;
	cout << "<@GLOBAL>regs_forwards:" << numForwards() << "</@GLOBAL>" << endl;
  }

  if(numReads_ == 0 && numWrites_ == 0) return;

  cout <<
    "Registers:\n" <<
    "  Reads:    " << numReads() << "\n" <<
    "  Writes:   " << numWrites() << "\n" <<
    "  Forwards: " << numForwards() << "\t(" << percentage(numForwards(),numReads()) << ")\n";
}
