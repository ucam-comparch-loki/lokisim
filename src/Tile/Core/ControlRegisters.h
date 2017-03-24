/*
 * ControlRegisters.h
 *
 * Documentation is in /usr/groups/comparch-loki/isa/encoding-spreadsheet.VERSION.ods
 *
 * As of version b6:
 *
 * 0  mask to control access to control registers in user/privileged modes?
 * 1  CPU location  Bits 3..0 – core ID within a tile, Bits 11..4 – tile ID (12-bit register)
 * 2
 * 3
 * 4  Count0cfg Bit 0 = counter0 enable, Bit 1 – count instructions/cycles
 * 5  Count1cfg Bit 0 = counter1 enable, Bit 1 – count instructions/cycles
 * 6  Count0    32-bit free running counter, increments every CPU clock cycle (if enabled)
 * 7  Compare0  See p.68 Let MIPS run, raise interrupt when compare==count
 * 8  Count1    32-bit free running counter, increments every CPU clock cycle (if enabled)
 * 9  Compare1  See p.68 Let MIPS run, raise interrupt when compare==count
 * 10 cp10      32-bit register
 * 11 cp11      32-bit register
 * 12 cp12      32-bit register
 * 13 cp13      32-bit register
 * 14
 * 15 throttle  Insert delays of N cycles after each instruction?
 *
 *  Created on: 7 Nov 2014
 *      Author: db434
 */

#ifndef CONTROLREGISTERS_H_
#define CONTROLREGISTERS_H_

#include "../../LokiComponent.h"

class ControlRegisters : public LokiComponent {

public:

  enum ControlRegister {
    CR_ACCESS_MASK      = 0,
    CR_CPU_LOCATION     = 1,
    CR_UNUSED2          = 2,
    CR_UNUSED3          = 3,
    CR_COUNT0_CONFIG    = 4,
    CR_COUNT1_CONFIG    = 5,
    CR_COUNT0           = 6,
    CR_COMPARE0         = 7,
    CR_COUNT1           = 8,
    CR_COMPARE1         = 9,
    CR_CP10             = 10,
    CR_CP11             = 11,
    CR_CP12             = 12,
    CR_CP13             = 13,
    CR_FETCH_ADDRESS    = 14,
    CR_THROTTLE         = 15
  };

  // Values to be stored in ACCESS_MASK register.
  enum AccessMask {
    ACCESS_USER = 0,
    ACCESS_PRIVILEGED = 1,
  };

  // Values to be stored in COUNT_CONFIG registers.
  enum CountConfig {
    CFG_DISABLE = 0,
    CFG_COUNT_CYCLES = 1,
    CFG_COUNT_INSTRUCTIONS = 3,
  };

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ControlRegisters);
  ControlRegisters(const sc_module_name& name, ComponentID id);
  virtual ~ControlRegisters();

//============================================================================//
// Methods
//============================================================================//

public:

  // Read from a control register.
  int32_t read(RegisterIndex reg) const;

  // Write to a control register.
  void write(RegisterIndex reg, int32_t value);

  // Signal that an instruction has been executed, which may possibly increment
  // a counter.
  // TODO: does this include predicated instructions?
  void instructionExecuted();

private:

  // Send an interrupt to the core.
  void interrupt();

  // Loop which increments relevant counters every cycle.
  void cycleCounter();

  // Return whether we are currently counting clock cycles.
  bool countingCycles() const;

  // Return whether we are currently counting instructions.
  bool countingInstructions() const;

//============================================================================//
// Local state
//============================================================================//

private:

  // Data array.
  vector<int32_t> registers;

  // Event triggered whenever registers 4 or 5 are updated to indicate that
  // clock cycles are to be counted.
  sc_event startCycleCount;

};

#endif /* CONTROLREGISTERS_H_ */
