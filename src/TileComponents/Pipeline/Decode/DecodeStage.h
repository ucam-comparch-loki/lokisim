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
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"
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
//   sc_in<bool>  clock
//   sc_out<bool> idle

  // The NUM_RECEIVE_CHANNELS inputs to the receive channel-end table.
  sc_in<Word>          *in;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS).
  sc_out<int>          *flowControlOut;

  // An output used to send FETCH requests.
  sc_out<AddressedWord> out1;

  // A flow control signal for the output.
  sc_in<bool>           flowControlIn;

  // The instruction to decode.
  sc_in<DecodedInst>    instructionIn;

  // The decoded instruction.
  sc_out<DecodedInst>   instructionOut;

  // Tells whether the execute stage is ready to receive a new operation.
  sc_in<bool>           readyIn;

  // Tell the fetch stage whether we are ready for a new instruction.
  sc_out<bool>          readyOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(DecodeStage);
  DecodeStage(sc_module_name name, ComponentID ID);
  virtual ~DecodeStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

  // Read a buffer in the receive channel end table. Warning: also removes
  // this value from the buffer.
  int32_t        readRCET(ChannelIndex index);

  // Request to refetch the pending instruction packet because it has been
  // overwritten since it was marked as being in the cache.
  void           refetch();

private:

  // Pass the given instruction to the decoder to be decoded.
  void           decode(const DecodedInst& i);

  // Send any pending data to the network (if possible) and update our output
  // flow control.
  void           updateReady();

  // The main loop: waiting until new input is received, and then acting on it.
  virtual void   execute();

  // Read a register value (for Decoder).
  int32_t        readReg(RegisterIndex index, bool indirect = false) const;

  // Get the value of the predicate register.
  bool           predicate() const;

  // Perform a TESTCH operation.
  bool           testChannel(ChannelIndex index) const;

  // Perform a SELCH operation.
  ChannelIndex   selectChannel();

  // Fetch an instruction packet from the given address.
  void           fetch(uint16_t addr);

  // Change the channel to which we send our fetch requests.
  void           setFetchChannel(ChannelID channelID);

  // Find out if the instruction packet from the given location is currently
  // in the instruction packet cache.
  bool           inCache(Address a);

  // Find out if there is room in the cache to fetch another packet.
  bool           roomToFetch() const;

  // Tell the instruction packet cache to jump to a new instruction.
  void           jump(int16_t offset);

  // Set the instruction packet cache into persistent or non-persistent mode.
  void           setPersistent(bool persistent);

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

};

#endif /* DECODESTAGE_H_ */
