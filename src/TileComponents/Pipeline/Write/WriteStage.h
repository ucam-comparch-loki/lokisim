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
//   sc_in<bool>  clock
//   sc_out<bool> idle

  // The result of an operation to be sent on the network and/or written to
  // a register.
  sc_in<DecodedInst>      result;

  // Tell the execute stage whether we are ready to receive new data.
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
  WriteStage(sc_module_name name, uint16_t ID);
  virtual ~WriteStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

private:

  // Set this pipeline stage up before execution begins.
  virtual void   initialise();

  // The task to be performed at the beginning of each clock cycle.
  virtual void   newCycle();

  // Write a new value to a register.
  void           writeReg(uint8_t reg, int32_t value, bool indirect = false);

//==============================//
// Components
//==============================//

private:

  SendChannelEndTable scet;

  friend class SendChannelEndTable;

};

#endif /* WRITESTAGE_H_ */
