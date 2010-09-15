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
//   clock
//   stall
//   idle

  // The NUM_RECEIVE_CHANNELS inputs to the receive channel-end table.
  sc_in<Word>          *in;

  // A flow control signal for each input (NUM_RECEIVE_CHANNELS).
  sc_out<int>          *flowControlOut;

  // An output used to send FETCH requests.
  sc_out<AddressedWord> out1;

  // A flow control signal for the output.
  sc_in<bool>           flowControlIn;

  // The instruction to decode.
  sc_in<Instruction>    instructionIn;

  // The decoded instruction.
  sc_out<DecodedInst>   instructionOut;

  // Signal telling whether the Execute stage is ready to receive a new
  // operation.
  sc_in<bool>           readyIn;

  // Tell the Fetch stage whether we are ready for a new instruction.
  sc_out<bool>          readyOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(DecodeStage);
  DecodeStage(sc_module_name name, int ID);
  virtual ~DecodeStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

  int32_t   readReg(uint8_t index, bool indirect = false) const;
  bool      predicate() const;
  int32_t   readRCET(ChannelIndex index);
  bool      testChannel(ChannelIndex index) const;
  ChannelIndex selectChannel();
  void      fetch(Address a);

  bool      inCache(Address a);
  bool      roomToFetch() const;
  void      jump(int8_t offset);
  void      setPersistent(bool persistent);

private:

  void decode(Instruction i);

  // The task performed at the beginning of each clock cycle.
  virtual void newCycle();

//==============================//
// Components
//==============================//

private:

  FetchLogic              fl;
  ReceiveChannelEndTable  rcet;
  Decoder                 decoder;
  SignExtend              extend;

//==============================//
// Local state
//==============================//

  DecodedInst             decoded;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_buffer<Word>        *fromNetwork;

};

#endif /* DECODESTAGE_H_ */
