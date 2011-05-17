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
    Bus* bus = new Bus(sc_gen_unique_name("bus"), i, numMuxes, level, size);
    buses.push_back(bus);
    bus->clock(clock);

    bus->dataIn[0](dataIn[i]);
    bus->validDataIn[0](validDataIn[i]);
    bus->ackDataIn[0](ackDataIn[i]);

    for(int j=0; j<numMuxes; j++) {
      bus->dataOut[j](busToMux[i][j]);
      bus->validDataOut[j](newData[i][j]);
      bus->ackDataOut[j](readData[i][j]);
    }
  }
}

void Crossbar::makeArbiters() {
  // Generate and connect up arbitrated multiplexers.
  for(int i=0; i<numMuxes; i++) {
    ArbiterComponent* arbiter = new ArbiterComponent(sc_gen_unique_name("arbiter"),
                                                     i, numBuses, outputsPerComponent);
    arbiters.push_back(arbiter);
    arbiter->clock(clock);

    for(int j=0; j<numBuses; j++) {
      arbiter->dataIn[j](busToMux[j][i]);
      arbiter->validDataIn[j](newData[j][i]);
      arbiter->ackDataIn[j](readData[j][i]);
    }

    for(int j=0; j<outputsPerComponent; j++) {
      arbiter->dataOut[j](dataOut[i*outputsPerComponent + j]);
      arbiter->validDataOut[j](validDataOut[i*outputsPerComponent + j]);
      arbiter->ackDataOut[j](ackDataOut[i*outputsPerComponent + j]);
    }
  }
}

Crossbar::Crossbar(sc_module_name name, const ComponentID& ID, int inputs, int outputs,
                   int outputsPerComponent, HierarchyLevel level, Dimension size,
                   bool externalConnection) :
    Network(name, ID, inputs, outputs, level, size, externalConnection),
    numBuses(numInputs),
    numMuxes(numOutputs/outputsPerComponent),
    outputsPerComponent(outputsPerComponent) {

  busToMux = new sc_signal<DataType>*[numBuses];
  newData  = new sc_signal<ReadyType>*[numBuses];
  readData = new sc_signal<ReadyType>*[numBuses];

  for(int i=0; i<numBuses; i++) {
    busToMux[i] = new sc_signal<DataType>[numMuxes];
    newData[i]  = new sc_signal<ReadyType>[numMuxes];
    readData[i] = new sc_signal<ReadyType>[numMuxes];
  }

}

Crossbar::~Crossbar() {
  for(int i=0; i<numBuses; i++) {
    delete[] busToMux[i];
    delete[] newData[i];
    delete[] readData[i];
  }

  delete[] busToMux;
  delete[] newData;
  delete[] readData;

  for(unsigned int i=0; i<buses.size(); i++) delete buses[i];
  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
}
