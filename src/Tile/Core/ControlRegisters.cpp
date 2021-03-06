/*
 * ControlRegisters.cpp
 *
 *  Created on: 7 Nov 2014
 *      Author: db434
 */

#include "ControlRegisters.h"

#include "../../Utility/Assert.h"

ControlRegisters::ControlRegisters(const sc_module_name& name, ComponentID id) :
    LokiComponent(name),
    registers(16, 0) {

  // Initialise CPU location.
  registers[CR_CPU_LOCATION] = id.flatten(Encoding::softwareComponentID);

  SC_METHOD(cycleCounter);

}

ControlRegisters::~ControlRegisters() {
  // Do nothing
}

int32_t ControlRegisters::read(RegisterIndex reg) const {
  loki_assert_with_message(reg < registers.size(), "Register %d", reg);
  // TODO: check access mask

  return registers[reg];
}

void ControlRegisters::write(RegisterIndex reg, int32_t value) {
  loki_assert_with_message(reg < registers.size(), "Register %d", reg);
  // TODO: check access mask

  // CPU location is read-only.
  // TODO: are any others read-only? The counters?
  assert(reg != CR_CPU_LOCATION);

  registers[reg] = value;

  if ((reg == CR_COUNT0_CONFIG || reg == CR_COUNT1_CONFIG) &&
      (value == CFG_COUNT_CYCLES))
    startCycleCount.notify();
}

void ControlRegisters::instructionExecuted() {
  if (registers[CR_COUNT0_CONFIG] == CFG_COUNT_INSTRUCTIONS) {
    registers[CR_COUNT0]++;
    if (registers[CR_COUNT0] == registers[CR_COMPARE0])
      interrupt();
  }
  if (registers[CR_COUNT1_CONFIG] == CFG_COUNT_INSTRUCTIONS) {
    registers[CR_COUNT1]++;
    if (registers[CR_COUNT1] == registers[CR_COMPARE1])
      interrupt();
  }
}

void ControlRegisters::interrupt() {
  // TODO
}

void ControlRegisters::cycleCounter() {
  if (!countingCycles())
    next_trigger(startCycleCount);
  else if (!clock.posedge())
    next_trigger(clock.posedge_event());
  else {
    if (registers[CR_COUNT0_CONFIG] == CFG_COUNT_CYCLES) {
      registers[CR_COUNT0]++;
      if (registers[CR_COUNT0] == registers[CR_COMPARE0])
        interrupt();
    }
    if (registers[CR_COUNT1_CONFIG] == CFG_COUNT_CYCLES) {
      registers[CR_COUNT1]++;
      if (registers[CR_COUNT1] == registers[CR_COMPARE1])
        interrupt();
    }
    next_trigger(clock.posedge_event());
  }
}

bool ControlRegisters::countingCycles() const {
  return (registers[CR_COUNT0_CONFIG] == CFG_COUNT_CYCLES) ||
         (registers[CR_COUNT1_CONFIG] == CFG_COUNT_CYCLES);
}

bool ControlRegisters::countingInstructions() const {
  return (registers[CR_COUNT0_CONFIG] == CFG_COUNT_INSTRUCTIONS) ||
         (registers[CR_COUNT1_CONFIG] == CFG_COUNT_INSTRUCTIONS);
}
