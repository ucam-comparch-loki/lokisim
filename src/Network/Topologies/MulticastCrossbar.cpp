/*
 * MulticastCrossbar.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#include "MulticastCrossbar.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../../Datatype/AddressedWord.h"

MulticastCrossbar::MulticastCrossbar(sc_module_name name, ComponentID ID, int inputs,
                                     int outputs, int outputsPerComponent,
                                     int channelsPerOutput, ChannelID startAddr,
                                     Dimension size) :
    Component(name, ID) {

  const int numBuses = inputs;
  const int numMuxes = outputs/outputsPerComponent;

  dataIn             = new DataInput[inputs];
  dataOut            = new DataOutput[outputs];
  creditsIn          = new CreditInput[outputs];
  creditsOut         = new CreditOutput[inputs];
  canSendData        = new ReadyInput[outputs];
  canReceiveData     = new ReadyOutput[inputs];
  canSendCredits     = new ReadyInput[inputs];
  canReceiveCredits  = new ReadyOutput[outputs];

  busToMux           = new sc_signal<DataType>[numBuses*numMuxes];
  newData            = new sc_signal<bool>[numBuses*numMuxes];

  // Generate and connect up buses.
  for(int i=0; i<numBuses; i++) {
    MulticastBus* bus = new MulticastBus("bus", i, numMuxes, channelsPerOutput, startAddr, size);
    buses.push_back(bus);
    bus->dataIn(dataIn[i]);
    bus->readyOut(canReceiveData[i]);

    for(int j=0; j<numMuxes; j++) {
      bus->dataOut[j](busToMux[i*numMuxes + j]);
      bus->newData[j](newData[i*numMuxes + j]);
    }
  }

  // Generate and connect up arbitrated multiplexers.
  for(int i=0; i<numMuxes; i++) {
    ArbiterComponent* arbiter = new ArbiterComponent("arbiter", i, numBuses, outputsPerComponent);
    arbiters.push_back(arbiter);
//    arbiter->clock(clock);

    for(int j=0; j<numBuses; j++) {
      arbiter->dataIn[j](busToMux[j*numMuxes + i]);
      arbiter->newData[j](newData[j*numMuxes + i]);
    }

    for(int j=0; j<outputsPerComponent; j++) {
      arbiter->dataOut[j](dataOut[i*outputsPerComponent + j]);
      arbiter->readyIn[j](canSendData[i*outputsPerComponent + j]);
    }
  }

  end_module();
}

MulticastCrossbar::~MulticastCrossbar() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] creditsIn;
  delete[] creditsOut;
  delete[] canSendData;
  delete[] canReceiveData;
  delete[] canSendCredits;
  delete[] canReceiveCredits;

  delete[] busToMux;
  delete[] newData;

  for(unsigned int i=0; i<buses.size(); i++) delete buses[i];
  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
}
