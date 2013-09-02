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
//   sc_out<bool>         idle
//   sc_out<bool>         stallOut

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  ReadyOutput              readyOut;

  // The NUM_RECEIVE_CHANNELS inputs to the receive channel-end table.
  LokiVector<sc_in<Word> > dataIn;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS).
  LokiVector<ReadyOutput>  flowControlOut;

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

  // Get the value of the predicate register.
  bool           predicate() const;

  // Retrieve the instruction's network destination from the channel map table,
  // if appropriate.
  void           readChannelMapTable(DecodedInst& inst) const;
  const ChannelMapEntry& channelMapTableEntry(MapIndex entry) const;

  // Perform a TESTCH operation.
  bool           testChannel(ChannelIndex index) const;

  // Perform a SELCH operation.
  ChannelIndex   selectChannel();

  const sc_event& receivedDataEvent(ChannelIndex buffer) const;

  // Find out if the instruction packet from the given location is currently
  // in the instruction packet cache.
  // There are many different ways of fetching instructions, so provide the
  // operation too.
  bool           inCache(const MemoryAddr addr, opcode_t operation) const;

  // Find out if there is room in the cache to fetch another packet, and if
  // there is not another fetch already in progress.
  bool           readyToFetch() const;

  // Tell the instruction packet cache to jump to a new instruction.
  void           jump(JumpOffset offset) const;

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
