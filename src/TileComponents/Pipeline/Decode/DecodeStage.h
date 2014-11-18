/*
 * DecodeStage.h
 *
 * Decode stage of the pipeline. Responsible for extracting useful information
 * from instructions and preparing it for use by the ALU, collecting data
 * inputs from the network, and sending memory requests to the network.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef DECODESTAGE_H_
#define DECODESTAGE_H_

#include "../PipelineStage.h"
#include "ReceiveChannelEndTable.h"
#include "Decoder.h"
#include "../../../Network/NetworkTypedefs.h"

class ChannelMapEntry;

class DecodeStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>          clock

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  ReadyOutput              oReady;

  // The NUM_RECEIVE_CHANNELS inputs to the receive channel-end table.
  LokiVector<sc_in<Word> > iData;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS).
  LokiVector<ReadyOutput>  oFlowControl;
  LokiVector<ReadyOutput>  oDataConsumed;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(DecodeStage);
  DecodeStage(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Read a buffer in the receive channel end table. Warning: also removes
  // this value from the buffer.
  int32_t        readRCET(ChannelIndex index);

  // Non-blocking read from the input buffers. Does not consume data.
  int32_t        readRCETDebug(ChannelIndex index) const;

  // If we are currently stalled waiting for input, stop waiting. Cancel the
  // current operation.
  void           unstall();

private:

  // The main loop controlling this stage. Involves waiting for new input,
  // doing work on it, and sending it to the next stage.
  virtual void   execute();

  // A second attempt at implementing execute(), but this time, avoiding the
  // use of expensive SC_THREADs by using a state machine.
//  virtual void   execute2();

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void   updateReady();

  // Repeatedly execute a single instruction until unstall() is called.
  void           persistentInstruction(DecodedInst& inst);

  // Pass the given instruction to the decoder to be decoded.
  virtual void   newInput(DecodedInst& inst);

  // Returns whether this stage is currently stalled (ignoring effects of any
  // other stages).
  virtual bool   isStalled() const;

  // Read a register value (for Decoder).
  int32_t        readReg(PortIndex port, RegisterIndex index, bool indirect = false) const;

  // Get the result from the instruction in the execute stage.
  int32_t        getForwardedData() const;

  // Get the value of the predicate register.
  bool           predicate(const DecodedInst& inst) const;

  // Retrieve the instruction's network destination from the channel map table,
  // if appropriate.
  void           readChannelMapTable(DecodedInst& inst);
  ChannelMapEntry& channelMapTableEntry(MapIndex entry) const;

  // Fetch the address requested by the instruction.
  void           fetch(const DecodedInst& inst);

  // Returns whether we are currently allowed to carry out a fetch request.
  bool           canFetch() const;

  // Returns whether we expect data arriving on a given channel to be from
  // memory. ChannelIndex 0 is mapped to the IPK FIFO.
  bool           connectionFromMemory(ChannelIndex channel) const;

  // Perform a TESTCH operation.
  bool           testChannel(ChannelIndex index) const;

  // Perform a SELCH operation.
  ChannelIndex   selectChannel(unsigned int bitmask, const DecodedInst& inst);

  const sc_event& receivedDataEvent(ChannelIndex buffer) const;

  // Tell the instruction packet cache to jump to a new instruction.
  void           jump(JumpOffset offset) const;

  // Signal from the Decoder telling that an instruction has been executed, and
  // will not continue further down the pipeline. Used to update control regs.
  void           instructionExecuted();

//==============================//
// Components
//==============================//

private:

  ReceiveChannelEndTable  rcet;
  Decoder                 decoder;

  friend class Decoder;
  friend class ReceiveChannelEndTable;

//==============================//
// Local state
//==============================//

private:

  enum DecodeState {
    INIT,
    NEW_INSTRUCTION,
    DECODE,
    WAIT_FOR_OPERANDS,
    SEND_RESULT
  };

  DecodeState state;

  bool startingNewPacket;

  bool waitingToSend;

};

#endif /* DECODESTAGE_H_ */
