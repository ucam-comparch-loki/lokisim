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

  // Fetch logic will sometimes provide messages to put into the output buffer.
  sc_in<AddressedWord>  fromFetchLogic;

  // Data to send onto the network.
  sc_out<AddressedWord> output;

  // A select signal to tell which network this data should be sent on.
  sc_out<int>           network;

  // Signal telling us whether we are able to send.
  sc_in<bool>           flowControl;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  sc_in<AddressedWord>  creditsIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name, ComponentID ID);

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

private:

  virtual void   execute();
  virtual void   newInput(DecodedInst& data);

  virtual bool   isStalled() const;

  // Write a new value to a register.
  void           writeReg(RegisterIndex reg, int32_t value, bool indirect = false) const;

//==============================//
// Components
//==============================//

private:

  SendChannelEndTable scet;

  friend class SendChannelEndTable;

};

#endif /* WRITESTAGE_H_ */
