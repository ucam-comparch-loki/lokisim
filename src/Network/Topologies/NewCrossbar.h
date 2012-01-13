/*
 * NewCrossbar.h
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#ifndef NEWCROSSBAR_H_
#define NEWCROSSBAR_H_

#include "../NewNetwork.h"

class BasicArbiter;
class Multiplexer;
class NewBus;

class NewCrossbar: public NewNetwork {

//==============================//
// Ports
//==============================//

public:

// Inherited from Network:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//  DataOutput   *dataOut;

  // A request/grant signal for each input to reserve each output.
  // Indexed as: requestsIn[input][output]
  RequestInput  **requestsIn;
  GrantOutput   **grantsOut;

  // A request/grant signal at each output to allow data to be sent.
  // This extra arbitration is not needed if the "chained" parameter is false.
  RequestOutput **requestsOut;
  GrantInput    **grantsIn;

  // A signal from each component telling whether it is ready to receive data.
  // These ports are not needed if the "chained" parameter is true.
  ReadyInput     *readyIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component
  // chained             = whether there needs to be a multiplexer after this
  //                       network, allowing many network to send to the same
  //                       destination
  NewCrossbar(const sc_module_name& name,
              const ComponentID& ID,
              int inputs,
              int outputs,
              int outputsPerComponent,
              HierarchyLevel level,
              Dimension size,
              bool chained = false);
  virtual ~NewCrossbar();

//==============================//
// Methods
//==============================//

private:

  void makePorts();
  void makeSignals();
  void makeArbiters();
  void makeBuses();
  void makeMuxes();

//==============================//
// Components
//==============================//

protected:

  const int numArbiters, numBuses, numMuxes;

  // The number of output ports which lead to the same component. Each
  // component will have one arbiter, but this many multiplexers.
  const int outputsPerComponent;

  // Tells whether this network is part of a chain: if so, further arbitration
  // will be needed after this, and so extra connections will be needed to
  // issue requests to the arbiter.
  const bool chained;

  std::vector<BasicArbiter*> arbiters;
  std::vector<NewBus*>       buses;
  std::vector<Multiplexer*>  muxes;

  DataSignal                *dataSig;
  SelectSignal             **selectSig;

};

#endif /* NEWCROSSBAR_H_ */
