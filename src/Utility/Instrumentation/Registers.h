/*
 * Registers.h
 *
 *  Created on: 21 Jan 2011
 *      Author: db434
 */

#ifndef REGISTERS_H_
#define REGISTERS_H_

#include "../../Datatype/ComponentID.h"
#include "InstrumentationBase.h"

namespace Instrumentation {

class Registers: public InstrumentationBase {

public:

// Maintenance
  static void init();
  static void end();

// Data logging
  static void write(RegisterIndex reg, int oldData, int newData);
  static void read(PortIndex port, RegisterIndex reg, int oldData, int newData);

  // Record the number of cycles in which register files were active. This
  // allows us to estimate the benefits of clock gating.
  static void activity();

  // Data was forwarded from the execute stage's output back to its input.
  // Since the wire may be quite long, do we want to collect the data too?
  static void forward(PortIndex port);

  // For use when the register file reads and writes to the same register in
  // the same clock cycle.
  static void bypass(PortIndex port);

// Data retrieval
  static count_t numReads();
  static count_t numWrites();
  static count_t numForwards();

  static count_t numReads(RegisterIndex reg);
  static count_t numWrites(RegisterIndex reg);

  static void printStats();
  static void dumpEventCounts(std::ostream& os);

private:

  static void valueSize(PortIndex port, int data);

public:

  // Set to true to record and print data beyond that needed for the energy
  // model. This includes a distribution of the sizes of values read and
  // written, and which registers are used.
  static bool detailedLogging;

private:

  enum RegisterPorts {WR, RD1, RD2};

// The variables needed for our energy model.
  // Total number of operations performed on each of the three ports.
  static count_t  operations[3];

  // Information about switching.
  static count_t  popCount[3], hammingDist[3];

  // Number of times data was read from a register in the same cycle as it was
  // written.
  static count_t  bypasses[3];

  static count_t  cyclesActive;

// Extra usage data which may be of interest.
  static count_t  numForwards_;
  static count_t* writesPerReg;
  static count_t* readsPerReg;

  // Record how large the values read/written are.
  // Each bin only records values which didn't fit in any previous bin.
  static count_t  zero[3],
                  uint8[3], uint16[3], uint24[3], uint32[3],
                  int8[3], int16[3], int24[3], int32[3];

};

}

#endif /* REGISTERS_H_ */
