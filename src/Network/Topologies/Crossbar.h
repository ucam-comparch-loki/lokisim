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
//
//  ReadyInput   *canSendData;
//  ReadyOutput  *canReceiveData;

//==============================//
// Methods
//==============================//

protected:

  virtual void makeBuses(int numBuses, int numArbiters, int channelsPerOutput, ChannelID startAddr);
  virtual void makeArbiters(int numBuses, int numArbiters, int outputsPerComponent);

//==============================//
// Constructors and destructors
//==============================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component
  // channelsPerOutput   = number of channel ends accessible through each output
  // startAddr           = the lowest channel ID accessible through a local output
  // externalConnection  = flag telling whether there is an extra input/output
  //                       connected to the next level of network hierarchy
  Crossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
           int outputsPerComponent, int channelsPerOutput, ChannelID startAddr,
           Dimension size, bool externalConnection = false);
  virtual ~Crossbar();

//==============================//
// Components
//==============================//

protected:

  const int numBuses, numMuxes, outputsPerComponent;

  std::vector<Bus*>              buses;
  std::vector<ArbiterComponent*> arbiters;

  // Two-dimensional arrays of signals. One of each signal goes from each bus
  // to each mux/arbiter.
  sc_signal<DataType>          **busToMux;
  sc_signal<ReadyType>         **newData;
  sc_signal<ReadyType>         **readData;

};

#endif /* CROSSBAR_H_ */
