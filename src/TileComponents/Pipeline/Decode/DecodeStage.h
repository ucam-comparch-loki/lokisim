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
#include "FetchLogic.h"
#include "ReceiveChannelEndTable.h"
#include "Decoder.h"
#include "SignExtend.h"

class DecodeStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>          clock
//   sc_out<bool>         idle
//   sc_out<bool>         stallOut

  // The input instruction to be working on. DecodedInst holds all information
  // required for any pipeline stage to do its work.
  sc_in<DecodedInst>    dataIn;

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  sc_out<bool>          readyOut;

  // The decoded instruction after passing through this pipeline stage.
  // DecodedInst holds all necessary fields for data at all stages throughout
  // the pipeline.
  sc_out<DecodedInst>   dataOut;

  // Since this stage can produce multiple outputs from a single input
  // instruction, it needs to be told when it can send data.
  sc_in<bool>           readyIn;

  // The NUM_RECEIVE_CHANNELS inputs to the receive channel-end table.
  sc_in<Word>          *rcetIn;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS).
  sc_out<bool>         *flowControlOut;

  // An output used to send FETCH requests.
  sc_out<AddressedWord> fetchOut;

  // A flow control signal for the output to the network.
  sc_in<bool>           flowControlIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(DecodeStage);
  DecodeStage(sc_module_name name, const ComponentID& ID);
  virtual ~DecodeStage();

//==============================//
// Methods
//==============================//

public:

  // Read a buffer in the receive channel end table. Warning: also removes
  // this value from the buffer.
  int32_t        readRCET(ChannelIndex index);

  // Request to refetch the pending instruction packet because it has been
  // overwritten since it was marked as being in the cache.
  void           refetch();

private:

  // The main loop controlling this stage. Involves waiting for new input,
  // doing work on it, and sending it to the next stage.
  virtual void   execute();

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void   updateReady();

  // Pass the given instruction to the decoder to be decoded.
  virtual void   newInput(DecodedInst& inst);

  // Returns whether this stage is currently stalled (ignoring effects of any
  // other stages).
  virtual bool   isStalled() const;

  // Read a register value (for Decoder).
  int32_t        readReg(RegisterIndex index, bool indirect = false) const;

  // Get the value of the predicate register.
  bool           predicate() const;

  // Perform a TESTCH operation.
  bool           testChannel(ChannelIndex index) const;

  // Perform a SELCH operation.
  ChannelIndex   selectChannel();

  // Fetch an instruction packet from the given address.
  void           fetch(const MemoryAddr addr);

  // Change the channel to which we send our fetch requests.
  void           setFetchChannel(const ChannelID& channelID, uint memoryGroupBits, uint memoryLineBits);

  // Find out if the instruction packet from the given location is currently
  // in the instruction packet cache.
  bool           inCache(const MemoryAddr a) const;

  // Find out if there is room in the cache to fetch another packet.
  bool           roomToFetch() const;

  // Tell the instruction packet cache to jump to a new instruction.
  void           jump(JumpOffset offset) const;

  // Set the instruction packet cache into persistent or non-persistent mode.
  void           setPersistent(bool persistent) const;

  // If an instruction is waiting to enter this pipeline stage, discard it.
  // Returns whether anything was discarded.
  bool           discardNextInst() const;

//==============================//
// Components
//==============================//

private:

  FetchLogic              fl;
  ReceiveChannelEndTable  rcet;
  Decoder                 decoder;
  SignExtend              extend;

  friend class FetchLogic;
  friend class Decoder;
  friend class ReceiveChannelEndTable;
  friend class SignExtend;

//==============================//
// Local state
//==============================//

  bool waitingToSend;

  sc_core::sc_event readyChangedEvent;

};

#endif /* DECODESTAGE_H_ */
