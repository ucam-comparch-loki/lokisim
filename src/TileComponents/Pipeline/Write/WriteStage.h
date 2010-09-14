/*
 * WriteStage.h
 *
 * Write stage of the pipeline. Responsible for sending the computation results
 * to be stored in registers, or out onto the network for another cluster or
 * memory to use.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef WRITESTAGE_H_
#define WRITESTAGE_H_

#include "../PipelineStage.h"
#include "SendChannelEndTable.h"

class WriteStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   clock
//   stall
//   idle

  sc_in<DecodedInst>      result;

  sc_out<bool>            readyOut;

  // The NUM_SEND_CHANNELS output channels.
  sc_out<AddressedWord>  *output;

  // A flow control signal for each output (NUM_SEND_CHANNELS).
  sc_in<bool>            *flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name);
  virtual ~WriteStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

  void writeReg(uint8_t reg, int32_t value, bool indirect = false);

private:

  // The task to be performed at the beginning of each clock cycle.
  virtual void   newCycle();

  // Generate a memory request using the address from the ALU and the operation
  // supplied by the decoder. The memory request will be sent to a memory and
  // will result in an operation being carried out there.
  Word getMemoryRequest() const;

//==============================//
// Components
//==============================//

private:

  SendChannelEndTable scet;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_buffer<Word>     ALUtoRegs, muxOutput;
  sc_buffer<Word>     ALUtoMux, instToMux;
  sc_buffer<short>    muxSelect;
  sc_buffer<short>    waitChannelSig;
  sc_signal<bool>     newInstSig, newDataSig;

};

#endif /* WRITESTAGE_H_ */
