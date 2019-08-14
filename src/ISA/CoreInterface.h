/*
 * CoreInterface.h
 *
 * Interface expected by instructions. Must be implemented by any core-like
 * component.
 *
 * All methods returning void must complete immediately, but may complete
 * asynchronously if necessary.
 *
 *  Created on: 2 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_COREINTERFACE_H_
#define SRC_ISA_COREINTERFACE_H_

#include "../Network/NetworkTypes.h"
#include "../Utility/SystemCall.h"

// The ISA supports a maximum of 2 register reads per cycle, and 2 operands per
// computation.
enum RegisterPort {
  REGISTER_PORT_1,
  REGISTER_PORT_2
};

const ChannelIndex ANY_CHANNEL = -1;

class ControlFlowInstruction;
class sc_event;

class CoreInterface {
protected:
  virtual ~CoreInterface() {}

public:
  // TODO: which of these can be made const?

  // Read the given register, and provide the output on the given port. The
  // register file is dual-ported, and each port is capable of supplying one
  // of the two ALU operands.
  // Call the instruction's `readRegistersCallback()` with the port and result.
  virtual void readRegister(RegisterIndex index, RegisterPort port) = 0;

  // Write the given value to the given register.
  // Call the instruction's `writeRegistersCallback()` when done.
  virtual void writeRegister(RegisterIndex index, int32_t value) = 0;

  // Read the value of the predicate bit.
  // Call the instruction's `readPredicateCallback()` with the result.
  virtual void readPredicate() = 0;

  // Write to the predicate bit.
  // Call the instruction's `writePredicateCallback()` when done.
  virtual void writePredicate(bool value) = 0;

  // Change the flow of instructions loaded by the Core.
  // `execute` = should the packet be executed? (e.g. prefetching => no)
  // `persistent` = should the packet be executed repeatedly?
  // TODO: does this need a callback?
  virtual void fetch(MemoryAddr address, ChannelMapEntry::MemoryChannel channel,
                     bool execute, bool persistent) = 0;

  // Implement latency of functional units.
  // Call the instruction's `computeCallback()` when finished.
  virtual void computeLatency(opcode_t opcode, function_t fn=(function_t)0) = 0;

  // Read a network mapping from the channel map table.
  // Call the instruction's `readCMTCallback()` with the result.
  virtual void readCMT(RegisterIndex index) = 0;

  // Write a network mapping to the channel map table.
  // Call the instruction's `writeCMTCallback()` when done.
  virtual void writeCMT(RegisterIndex index, EncodedCMTEntry value) = 0;

  // Read the given control register.
  // Call the instruction's `readCregsCallback()` with the result.
  virtual void readCreg(RegisterIndex index) = 0;

  // Write the given control register.
  // Call the instruction's `writeCregsCallback()` when done.
  virtual void writeCreg(RegisterIndex index, int32_t value) = 0;

  // Read from the core-local scratchpad.
  // Call the instruction's `readScratchpadCallback()` with the result.
  virtual void readScratchpad(RegisterIndex index) = 0;

  // Write to the core-local scratchpad.
  // Call the instruction's `writeScratchpadCallback()` when done.
  virtual void writeScratchpad(RegisterIndex index, int32_t value) = 0;

  // Execute a system call.
  // Call the instruction's `computeCallback()` when finished.
  virtual void syscall(int code) = 0;

  // Wait for a credit to arrive at the given output channel.
  // Call the current instruction's `computeCallback()` when a credit arrives.
  virtual void waitForCredit(ChannelIndex channel) const = 0;

  // Send a flit onto the network.
  // Call the instruction's `sendNetworkDataCallback()` when the flit has safely
  // been sent or buffered.
  virtual void sendOnNetwork(NetworkData flit) = 0;

  // From the input buffers identified in the bitmask, select one with data. If
  // all are empty, wait until data arrives.
  // Call the instruction's `computeCallback()` with the result.
  virtual void selectChannelWithData(uint bitmask) = 0;


  // The following methods complete immediately.

  // Begin remote execution. All following instructions will be forwarded
  // directly to `address` without being executed locally. This will only stop
  // when `endRemoteExecution` is called.
  virtual void startRemoteExecution(ChannelID address) = 0;

  // End remote execution and resume local execution of instructions.
  virtual void endRemoteExecution() = 0;

  // Jump within the current instruction source by the given number of
  // instructions. This avoids tag checks so can be more efficient than a fetch.
  virtual void jump(JumpOffset offset) = 0;

  // Determine which network address the given channel map entry corresponds to.
  // If the address is a memory bank, the memory address is also needed to
  // select one bank out of a group.
  virtual ChannelID getNetworkDestination(EncodedCMTEntry channelMap,
                                          MemoryAddr address=0) const = 0;

  // Returns whether the given FIFO has data ready to read. Channel indices
  // correspond to the network address used to access that channel.
  // ANY_CHANNEL may also be used, in which case the function returns whether
  // any input channel has data.
  virtual bool inputFIFOHasData(ChannelIndex fifo) const = 0;

  // Return the number of credits available to the given output channel.
  virtual uint creditsAvailable(ChannelIndex channel) const = 0;

};



#endif /* SRC_ISA_COREINTERFACE_H_ */
