/*
 * StallRegister.h
 *
 * Two registers to go between each pair of pipeline stages, removing the need
 * for instantaneous propagation of pipeline flow control signals.
 *
 * Loosely based (much higher-level) on the implementation in
 * http://www.cl.cam.ac.uk/~rdm34/Memos/localstall.pdf
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#ifndef STALLREGISTER_H_
#define STALLREGISTER_H_

#include "../../Component.h"
#include "../../Memory/BufferStorage.h"

class DecodedInst;

class StallRegister: public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock.
  sc_in<bool>         clock;

  // Tells whether the local pipeline stage (the one immediately following this
  // stall register) is stalling.
  sc_in<bool>         localStageReady;

  // Tells whether the next stall register is ready to receive new data.
  sc_in<bool>         readyIn;

  // Tell the previous stall register whether we are ready to receive new data.
  sc_out<bool>        readyOut;

  // Data from the previous pipeline stage.
  sc_in<DecodedInst>  dataIn;

  // Data sent to the following pipeline stage.
  sc_out<DecodedInst> dataOut;

//==============================//
// Methods
//==============================//

  // Discard the oldest instruction in this StallRegister (if any). Returns
  // whether anything was discarded.
  bool discard();

private:

  // Send any held data at the beginning of each clock cycle.
  void newCycle();

  // Store any newly-received data in the registers.
  void newData();

  // Propagate the "ready" signal to the previous pipeline stage.
  void receivedReady();

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(StallRegister);  // Tell SystemC that this component has its
                                  // own methods which react to inputs.
  StallRegister(const sc_module_name& name, const ComponentID& ID);

//==============================//
// Local state
//==============================//

private:

  bool ready;

  // My implementation of the two registers.
  BufferStorage<DecodedInst> buffer;

  // Event which is triggered whenever the buffer becomes or stops being
  // completely full.
  sc_core::sc_event bufferFillChanged;

  // Event which is triggered whenever any data is added to or removed from
  // the buffer.
  sc_core::sc_event bufferContentsChanged;

};

#endif /* STALLREGISTER_H_ */
