/*
 * Crossbar.cpp
 *
 *  Created on: 21 Mar 2011
 *      Author: db434
 */

#include "Crossbar.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../../Datatype/AddressedWord.h"

void Crossbar::initialise() {
  makeBuses();
  makeArbiters();
}

void Crossbar::makeBuses() {
  // Generate and connect up buses.
  for(int i=0; i<numBuses; i++) {
    Bus* bus = new Bus(sc_gen_unique_name("bus"), i, numMuxes, level);
    buses.push_back(bus);
    bus->clock(clock);

    bus->dataIn[0](dataIn[i]);

    for(int j=0; j<numMuxes; j++) {
      bus->dataOut[j](busToMux[i][j]);
    }
  }
}

void Crossbar::makeArbiters() {
  // Generate and connect up arbitrated multiplexers.
  for(int i=0; i<numMuxes; i++) {
    ArbiterComponent* arbiter = new ArbiterComponent(sc_gen_unique_name("arbiter"),
                                                     i, numBuses, outputsPerComponent, false);
    arbiters.push_back(arbiter);
    arbiter->clock(clock);

    for(int j=0; j<numBuses; j++)
      arbiter->dataIn[j](busToMux[j][i]);

    for(int j=0; j<outputsPerComponent; j++)
      arbiter->dataOut[j](dataOut[i*outputsPerComponent + j]);
  }
}

Crossbar::Crossbar(sc_module_name name, const ComponentID& ID, int inputs, int outputs,
                   int outputsPerComponent, HierarchyLevel level, bool externalConnection) :
    Network(name, ID, inputs, outputs, level, 0, externalConnection),
    numBuses(numInputPorts()),
    numMuxes(numOutputPorts()/outputsPerComponent),
    outputsPerComponent(outputsPerComponent) {

  busToMux.init(numBuses, numMuxes);

}

Crossbar::~Crossbar() {
  for(unsigned int i=0; i<buses.size(); i++) delete buses[i];
  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
}
