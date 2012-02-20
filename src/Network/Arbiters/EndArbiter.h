/*
 * EndArbiter.h
 *
 * An arbiter which directly accesses a core or memory's input ports.
 *
 * Can be used to complete a chain of ChainedArbiters, where each arbiter
 * except this one must itself issue a request to the next arbiter.
 *
 *  Created on: 6 Sep 2011
 *      Author: db434
 */

#ifndef ENDARBITER_H_
#define ENDARBITER_H_

#include "BasicArbiter.h"

class EndArbiter: public BasicArbiter {

//==============================//
// Ports
//==============================//

public:

// Inherited from BasicArbiter:
//  sc_in<bool>   clock;
//  RequestInput *requests;
//  GrantOutput  *grants;
//  SelectOutput *select;

  // Signals from the component telling if it is able to receive more data.
  ReadyInput *readyIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  EndArbiter(const sc_module_name& name, ComponentID ID,
             int inputs, int outputs, bool wormhole, int flowControlSignals);
  virtual ~EndArbiter();

//==============================//
// Methods
//==============================//

protected:

  virtual const sc_event& canGrantNow(int output, const ChannelIndex destination);
  virtual const sc_event& stallGrant(int output);

  // The task to perform whenever a request input changes.
  virtual void requestChanged(int input);

private:

  // Return which ready signal dictates when the input data will be allowed
  // to send.
  PortIndex outputToUse(PortIndex input);

//==============================//
// Methods
//==============================//

private:

  // The number of flow control signals this arbiter receives.
  const int flowControlSignals;

  // An event which is notified whenever canGrantNow determines that it is safe
  // to grant a request.
  sc_event grantEvent;

};

#endif /* ENDARBITER_H_ */
