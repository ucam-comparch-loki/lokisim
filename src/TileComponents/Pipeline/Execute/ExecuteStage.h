/*
 * ExecuteStage.h
 *
 * Execute stage of the pipeline. Responsible for transforming the given
 * data and outputting the result.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef EXECUTESTAGE_H_
#define EXECUTESTAGE_H_

#include "../PipelineStage.h"
#include "../../../Network/NetworkTypedefs.h"
#include "../../../Utility/Blocking.h"
#include "ALU.h"
#include "Scratchpad.h"

class ExecuteStage: public PipelineStage, public Blocking {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   ClockInput          clock

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  ReadyOutput         oReady;

  // Data to be sent over the network.
  DataOutput          oData;
  ReadyInput          iReady;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ExecuteStage);
  ExecuteStage(const sc_module_name& name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // An event which is triggered whenever execution of an instruction completes.
  const sc_event& executedEvent() const;

private:

  // The main loop controlling this stage. Involves waiting for new input,
  // doing work on it, and sending it to the next stage.
  virtual void execute();

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void updateReady();

  // The task performed when a new operation is received.
  virtual void newInput(DecodedInst& operation);
  void sendOutput();

  // Send a request to reserve (or release) a connection to a particular
  // destination component. May cause re-execution of the calling method when
  // the request is granted.
  void requestArbitration(ChannelID destination, bool request);

  void setChannelMap(DecodedInst& operation);

  // Compute memory address + determine which bank to access.
  void memoryStorePhase1(DecodedInst& operation);
  // Send data to store.
  void memoryStorePhase2(DecodedInst& operation);

  // Determine which memory bank from a group should be accessed.
  void adjustNetworkAddress(DecodedInst& operation) const;

  virtual bool isStalled() const;

  // Read the predicate register.
  bool readPredicate() const;
  int32_t readReg(RegisterIndex reg) const;
  int32_t readWord(MemoryAddr addr) const;
  int32_t readByte(MemoryAddr addr) const;

  // Write to the predicate register.
  void writePredicate(bool val) const;
  void writeReg(RegisterIndex reg, Word data) const;
  void writeWord(MemoryAddr addr, Word data) const;
  void writeByte(MemoryAddr addr, Word data) const;

  // Check the predicate bits of this instruction and the predicate register to
  // see if this instruction should execute.
  bool checkPredicate(DecodedInst& inst);

  // Compute the new value of the predicate and update it accordingly.
  void updatePredicate(const DecodedInst& inst);

protected:

  virtual void reportStalls(ostream& os);

//==============================//
// Components
//==============================//

private:

  ALU alu;
  Scratchpad scratchpad;

  friend class ALU;
  friend class Scratchpad;

//==============================//
// Local state
//==============================//

private:

  // The result of the previous instruction to be forwarded, if necessary.
  int32_t forwardedResult;

  // Don't use data forwarding if the previous instruction didn't execute.
  bool previousInstExecuted;

  // Don't read a new instruction from the pipeline register if we are blocked;
  // just continue executing the same instruction.
  bool blocked;

  // Store operations take two cycles. This flag is true when working on the
  // second half.
  bool continuingStore;

};

#endif /* EXECUTESTAGE_H_ */
