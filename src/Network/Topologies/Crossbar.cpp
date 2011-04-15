/*
 * Crossbar.cpp
 *
 *  Created on: 21 Mar 2011
 *      Author: db434
 */

#include "Crossbar.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../../Datatype/AddressedWord.h"

void Crossbar::makeBuses(int numBuses, int numArbiters, int channelsPerOutput, ChannelID startAddr) {
  // Generate and connect up buses.
  for(int i=0; i<numBuses; i++) {
    Bus* bus = new Bus("bus", i, numArbiters, numOutputs*channelsPerOutput, startAddr, size);
    buses.push_back(bus);

    bus->dataIn(dataIn[i]);
    bus->validDataIn(validDataIn[i]);
    bus->ackDataIn(ackDataIn[i]);

    for(int j=0; j<numArbiters; j++) {
      bus->dataOut[j](busToMux[i][j]);
      bus->validDataOut[j](newData[i][j]);
      bus->ackDataOut[j](readData[i][j]);
    }
  }
}

void Crossbar::makeArbiters(int numBuses, int numArbiters, int outputsPerComponent) {
  // Generate and connect up arbitrated multiplexers.
  for(int i=0; i<numArbiters; i++) {
    ArbiterComponent* arbiter = new ArbiterComponent("arbiter", i, numBuses, outputsPerComponent);
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

Crossbar::Crossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
                   int outputsPerComponent, int channelsPerOutput,
                   ChannelID startAddr, Dimension size, bool externalConnection) :
    Network(name, ID, inputs, outputs, size, externalConnection),
    numBuses(numInputs),
    numMuxes(numOutputs/outputsPerComponent),
    outputsPerComponent(outputsPerComponent) {

  id = ID;

//  const int numBuses = numInputs;
//  const int numMuxes = numOutputs/outputsPerComponent;

  busToMux = new sc_signal<DataType>*[numBuses];
  newData  = new sc_signal<ReadyType>*[numBuses];
  readData = new sc_signal<ReadyType>*[numBuses];

  for(int i=0; i<numBuses; i++) {
    busToMux[i] = new sc_signal<DataType>[numMuxes];
    newData[i]  = new sc_signal<ReadyType>[numMuxes];
    readData[i] = new sc_signal<ReadyType>[numMuxes];
  }

  makeBuses(numBuses, numMuxes, channelsPerOutput, startAddr);
  makeArbiters(numBuses, numMuxes, outputsPerComponent);

  end_module();

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
