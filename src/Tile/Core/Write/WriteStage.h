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

#include "../../../Network/NetworkTypes.h"
#include "../PipelineStage.h"
#include "SendChannelEndTable.h"

class WriteStage: public PipelineStage {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from PipelineStage:
//   sc_in<bool>         clock
//   sc_out<bool>        idle
//   sc_out<bool>        stallOut

  // Data (with an address) to be sent over the network.
  // Data can arrive at any time.
  DataInput               iFetch;
  DataInput               iData;

  // Tell whether this stage is ready for input (ignoring effects of any other stages).
  ReadyOutput             oReady;

  // Data to send onto the network.
  DataOutput              oDataLocal;
  DataOutput              oDataMemory;
  DataOutput              oDataGlobal;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  CreditInput             iCredit;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(WriteStage);
  WriteStage(sc_module_name name, const fifo_parameters_t& fifoParams);

//============================================================================//
// Methods
//============================================================================//

public:

  // Deliver a credit while bypassing the network.
  void           deliverCreditInternal(const NetworkCredit& credit);

private:

  virtual void   execute();
  virtual void   newInput(DecodedInst& inst);

  // Determine whether this stage is stalled or not, and write the appropriate
  // output.
  virtual void   updateReady();

  virtual bool   isStalled() const;

  // Write a new value to a register.
  void           writeReg(RegisterIndex reg, int32_t value, bool indirect = false) const;

  void           requestArbitration(ChannelID destination, bool request);
  bool           requestGranted(ChannelID destination) const;

//============================================================================//
// Components
//============================================================================//

private:

  SendChannelEndTable scet;

  friend class SendChannelEndTable;

};

#endif /* WRITESTAGE_H_ */
