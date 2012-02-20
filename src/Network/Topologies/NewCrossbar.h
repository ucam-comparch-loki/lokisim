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

  // A signal from each buffer of each component, telling whether it is ready
  // to receive data. Addressed using readyIn[component][buffer].
  ReadyInput    **readyIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component.
  // buffersPerComponent = the number of buffers each destination component has.
  //                       There will be one flow control signal per buffer.
  NewCrossbar(const sc_module_name& name,
              const ComponentID& ID,
              int inputs,
              int outputs,
              int outputsPerComponent,
              HierarchyLevel level,
              Dimension size,
              int buffersPerComponent);
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

  // We will need a flow control signal from each buffer of each component.
  // This value tells how many there are.
  const int buffersPerComponent;

  std::vector<BasicArbiter*> arbiters;
  std::vector<NewBus*>       buses;
  std::vector<Multiplexer*>  muxes;

  DataSignal                *dataSig;
  SelectSignal             **selectSig;

};

#endif /* NEWCROSSBAR_H_ */
