/*
 * NormalCrossbar.cpp
 *
 *  Created on: 21 Mar 2011
 *      Author: db434
 */

#include "NormalCrossbar.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../../Datatype/AddressedWord.h"

NormalCrossbar::NormalCrossbar(sc_module_name name, ComponentID ID, int inputs,
                               int outputs, int outputsPerComponent,
                               int channelsPerOutput, ChannelID startAddr) :
    Component(name) {

  id = ID;

  const int numBuses = inputs;
  const int numMuxes = outputs/outputsPerComponent;

  dataIn             = new DataInput[inputs];
  dataOut            = new DataOutput[outputs];
  canSendData        = new ReadyInput[outputs];
  canReceiveData     = new ReadyOutput[inputs];

  busToMux           = new sc_signal<DataType>[numBuses*numMuxes];
  newData            = new sc_signal<bool>[numBuses*numMuxes];
  readData           = new flag_signal<bool>[numBuses*numMuxes];

  // Generate and connect up buses.
  for(int i=0; i<numBuses; i++) {
    Bus* bus = new Bus("bus", i, numMuxes, channelsPerOutput, startAddr);
    buses.push_back(bus);
    bus->dataIn(dataIn[i]);
    bus->readyOut(canReceiveData[i]);

    for(int j=0; j<numMuxes; j++) {
      bus->dataOut[j](busToMux[i*numMuxes + j]);
      bus->newData[j](newData[i*numMuxes + j]);
      bus->dataRead[j](readData[i*numMuxes + j]);
    }
  }

  // Generate and connect up arbitrated multiplexers.
  for(int i=0; i<numMuxes; i++) {
    ArbiterComponent* arbiter = new ArbiterComponent("arbiter", i, numBuses, outputsPerComponent);
    arbiters.push_back(arbiter);
    arbiter->clock(clock);

    for(int j=0; j<numBuses; j++) {
      arbiter->dataIn[j](busToMux[j*numMuxes + i]);
      arbiter->newData[j](newData[j*numMuxes + i]);
      arbiter->readData[j](readData[j*numMuxes + i]);
    }

    for(int j=0; j<outputsPerComponent; j++) {
      arbiter->dataOut[j](dataOut[i*outputsPerComponent + j]);
      arbiter->readyIn[j](canSendData[i*outputsPerComponent + j]);
    }
  }

}

NormalCrossbar::~NormalCrossbar() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] canSendData;
  delete[] canReceiveData;

  delete[] busToMux;
  delete[] newData;
  delete[] readData;

  for(unsigned int i=0; i<buses.size(); i++) delete buses[i];
  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
}
