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

#include "../StageWithPredecessor.h"
#include "SendChannelEndTable.h"

class WriteStage: public StageWithPredecessor {

//==============================//
// Ports
//==============================//

public:

// Inherited from StageWithPredecessor:
//   sc_in<bool>        clock
//   sc_out<bool>       idle
//   sc_in<DecodedInst> dataIn
//   sc_out<bool>       stallOut

  // The NUM_SEND_CHANNELS output channels.
  sc_out<AddressedWord>  *output;

  // A flow control signal for each output (NUM_SEND_CHANNELS).
  sc_in<bool>            *flowControl;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name, ComponentID ID);
  virtual ~WriteStage();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

private:

  virtual void   newInput(DecodedInst& data);

  virtual bool   isStalled() const;

  virtual void   sendOutputs();

  // Write a new value to a register.
  void           writeReg(RegisterIndex reg, int32_t value, bool indirect = false);

//==============================//
// Components
//==============================//

private:

  SendChannelEndTable scet;

  friend class SendChannelEndTable;

};

#endif /* WRITESTAGE_H_ */
