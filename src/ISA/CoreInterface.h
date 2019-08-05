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
  virtual void readRegister(RegisterIndex index, RegisterPort port) = 0;

  // Write the given value to the given register.
  virtual void writeRegister(RegisterIndex index, int32_t value) = 0;

  // Read the value of the predicate bit.
  virtual void readPredicate() = 0;

  // Write to the predicate bit.
  virtual void writePredicate(bool value) = 0;

  // Change the flow of instructions loaded by the Core. The whole instruction
  // is passed to allow complex control of the process.
  virtual void fetch(ControlFlowInstruction& inst) = 0;

  // Read a network mapping from the channel map table.
  virtual void readCMT(RegisterIndex index) = 0;

  // Write a network mapping to the channel map table.
  virtual void writeCMT(RegisterIndex index, EncodedCMTEntry value) = 0;

  // Begin remote execution. All following instructions will be forwarded
  // directly to `address` without being executed locally. This will only stop
  // when `endRemoteExecution` is called.
  virtual void startRemoteExecution(ChannelID address) = 0;

  // End remote execution and resume local execution of instructions.
  virtual void endRemoteExecution() = 0;

  // Read the given control register.
  virtual void readCreg(RegisterIndex index) = 0;

  // Write the given control register.
  virtual void writeCreg(RegisterIndex index, int32_t value) = 0;

  // Read from the core-local scratchpad.
  virtual void readScratchpad(RegisterIndex index) = 0;

  // Write to the core-local scratchpad.
  virtual void writeScratchpad(RegisterIndex index, int32_t value) = 0;

  // Execute a system call.
  virtual void syscall(SystemCall code) = 0;

  // Send a flit onto the network.
  virtual void sendOnNetwork(NetworkData flit) = 0;


  // The following methods return results immediately.

  // Determine which network address should be used to access the given memory
  // address. This allows multiple memory banks to be grouped together.
  virtual ChannelID getNetworkDestination(EncodedCMTEntry channelMap,
                                          MemoryAddr address) const = 0;

  // Returns whether the given FIFO has data ready to read. Channel indices
  // correspond to the network address used to access that channel.
  // ANY_CHANNEL may also be used, in which case the function returns whether
  // any input channel has data.
  virtual bool inputFIFOHasData(ChannelIndex fifo) const = 0;

  // Event triggered whenever the given FIFO receives data.
  // ANY_CHANNEL may also be used, in which case the function returns an event
  // which is triggered when any input channel receives data.
  virtual const sc_event& inputFIFOReceivedDataEvent(ChannelIndex fifo) const = 0;

  // Return whether the given output channel has fewer than the maximum number
  // of credits.
  virtual bool waitingForCredits(ChannelIndex channel) const = 0;

  // Event triggered whenever the given output channel receives a new credit.
  virtual const sc_event& creditArrivedEvent(ChannelIndex channel) const = 0;

};



#endif /* SRC_ISA_COREINTERFACE_H_ */
