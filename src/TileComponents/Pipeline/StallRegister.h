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
#include "../../Memory/Buffer.h"

class DecodedInst;

class StallRegister: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>         clock;

  sc_in<bool>         readyIn;
  sc_out<bool>        readyOut;

  sc_in<DecodedInst>  dataIn;
  sc_out<DecodedInst> dataOut;

//==============================//
// Methods
//==============================//

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
  StallRegister(sc_module_name name);
  virtual ~StallRegister();

//==============================//
// Local state
//==============================//

private:

  // My implementation of the two registers.
  Buffer<DecodedInst> buffer;

};

#endif /* STALLREGISTER_H_ */
