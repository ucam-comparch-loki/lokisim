/*
 * InterclusterNetwork.h
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERCLUSTERNETWORK_H_
#define INTERCLUSTERNETWORK_H_

#include "Interconnect.h"

class InterclusterNetwork: public Interconnect {

public:
/* Ports */
  sc_in<AddressedWord> *requestsIn,  *responsesIn,  *dataIn;
  sc_out<Word>         *requestsOut, *responsesOut, *dataOut;
  // Some AddressedWord outputs for longer communications?

/* Constructors and destructors */
  SC_HAS_PROCESS(InterclusterNetwork);
  InterclusterNetwork(sc_module_name name);
  virtual ~InterclusterNetwork();

protected:
/* Methods */
  virtual void routeRequests();
  virtual void routeResponses();
  virtual void routeData();
  virtual void route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs);

/* Local state */
  int numInputs, numOutputs;

};

#endif /* INTERCLUSTERNETWORK_H_ */
