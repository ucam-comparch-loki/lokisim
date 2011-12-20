/*
 * Crossbar.h
 *
 * A crossbar without multicast functionality.
 *
 *  Created on: 21 Mar 2011
 *      Author: db434
 */

#ifndef CROSSBAR_H_
#define CROSSBAR_H_

#include "../Network.h"
#include "Bus.h"

class ArbiterComponent;

class Crossbar: public Network {

//==============================//
// Ports
//==============================//

// Inherited from Network:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//  DataOutput   *dataOut;

//==============================//
// Methods
//==============================//

public:

  // Create the buses and arbiters. Note that this must be called from outside
  // the constructor, as it will be calling virtual methods.
  void initialise();

protected:

  virtual void makeBuses();
  virtual void makeArbiters();

//==============================//
// Constructors and destructors
//==============================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component
  // externalConnection  = flag telling whether there is an extra input/output
  //                       connected to the next level of network hierarchy
  Crossbar(sc_module_name name, const ComponentID& ID, int inputs, int outputs,
           int outputsPerComponent, HierarchyLevel level, Dimension size,
           bool externalConnection = false);
  virtual ~Crossbar();

//==============================//
// Components
//==============================//

protected:

  // Parameters of this crossbar.
  const int numBuses, numMuxes;
  const int outputsPerComponent;

  std::vector<Bus*>              buses;
  std::vector<ArbiterComponent*> arbiters;

  // Two-dimensional arrays of signals. One of each signal goes from each bus
  // to each mux/arbiter.
  DataSignal        **busToMux;

};

#endif /* CROSSBAR_H_ */
