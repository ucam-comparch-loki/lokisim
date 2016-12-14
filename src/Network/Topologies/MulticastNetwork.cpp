/*
 * MulticastNetwork.cpp
 *
 *  Created on: 25 Jul 2012
 *      Author: db434
 */

#include "MulticastNetwork.h"
#include "../../Utility/Instrumentation/Network.h"

void MulticastNetwork::inputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::multicastTraffic(oldInputs[port], iData[port].read(), port);
  oldInputs[port] = iData[port].read();
}

void MulticastNetwork::outputChanged(const PortIndex port) {
  // No instrumentation needed for output ports.
//  if (ENERGY_TRACE)
//    Instrumentation::Network::crossbarOutput(oldOutputs[port], dataOut[port].read());
//  oldOutputs[port] = dataOut[port].read();
}

MulticastNetwork::MulticastNetwork(const sc_module_name& name,
                                   const ComponentID& ID,
                                   int inputs,
                                   int outputs,
                                   int outputsPerComponent,
                                   int buffersPerComponent) :
  Crossbar(name, ID, inputs, outputs, outputsPerComponent, COMPONENT, buffersPerComponent){

  // Everything is done for us by Crossbar.

}

MulticastNetwork::~MulticastNetwork() {

}
