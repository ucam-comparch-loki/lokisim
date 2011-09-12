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
//  sc_in<bool>  *requests;
//  sc_out<bool> *grants;
//  sc_out<int>  *select;

//==============================//
// Constructors and destructors
//==============================//

public:

  EndArbiter(const sc_module_name& name, ComponentID ID,
             int inputs, int outputs, bool wormhole);

//==============================//
// Methods
//==============================//

protected:

  virtual const sc_event& canGrantNow(int output);

//==============================//
// Methods
//==============================//

private:

  // An event which is notified whenever canGrantNow determines that it is safe
  // to grant a request.
  sc_event grantEvent;

};

#endif /* ENDARBITER_H_ */
