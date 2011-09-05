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
#include "../../ChannelMapEntry.h"
#include "../../../Datatype/DecodedInst.h"

class WriteStage: public PipelineStage {

//==============================//
// Ports
//==============================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>         clock
//   sc_out<bool>        idle
//   sc_out<bool>        stallOut

  // The input instruction to be working on. DecodedInst holds all information
  // required for any pipeline stage to do its work.
  sc_in<DecodedInst>     dataIn;

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  sc_out<bool>           readyOut;

  // Data to send onto the network.
  sc_out<AddressedWord> *output;
  sc_out<bool>          *validOutput;
  sc_in<bool>           *ackOutput;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  sc_in<AddressedWord>  *creditsIn;
  sc_in<bool>           *validCredit;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name, const ComponentID& ID);
  virtual ~WriteStage();

//==============================//
// Methods
//==============================//

public:

  // The instruction whose result is currently being written to registers or
  // the network. Used to determine whether forwarding is required.
  const DecodedInst& currentInstruction() const;

private:

  virtual void   execute();
  virtual void   newInput(DecodedInst& data);

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void   updateReady();

  virtual bool   isStalled() const;

  // Write a new value to a register.
  void           writeReg(RegisterIndex reg, int32_t value, bool indirect = false) const;

//==============================//
// Components
//==============================//

private:

  SendChannelEndTable scet;

  friend class SendChannelEndTable;

//==============================//
// Local state
//==============================//

private:

  bool endOfPacket;

  // The instruction whose result is currently being written to registers or
  // the network. Used to determine whether forwarding is required.
  DecodedInst currentInst;

};

#endif /* WRITESTAGE_H_ */
