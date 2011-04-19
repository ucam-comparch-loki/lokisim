/*
 * MulticastCrossbar.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#include "MulticastCrossbar.h"
#include "MulticastBus.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../../Datatype/AddressedWord.h"

void MulticastCrossbar::makeBuses(int numBuses, int numArbiters, int channelsPerOutput, ChannelID startAddr) {
  // Generate and connect up buses.
  for(int i=0; i<numBuses; i++) {
    MulticastBus* bus = new MulticastBus("bus", i, numArbiters, numOutputs*channelsPerOutput, startAddr, size);
    buses.push_back(bus);

    bus->dataIn(dataIn[i]);
    bus->validDataIn(validDataIn[i]);
    bus->ackDataIn(ackDataIn[i]);
    bus->creditsOut(creditsOut[i]);
    bus->canSendCredits(ackCreditOut[i]);

    for(int j=0; j<numArbiters; j++) {
      bus->dataOut[j](busToMux[i][j]);
      bus->validDataOut[j](newData[i][j]);
      bus->ackDataOut[j](readData[i][j]);
      bus->creditsIn[j](creditsToBus[i][j]);
      bus->canReceiveCredits[j](busReadyCredits[i][j]);
    }
  }
}

void MulticastCrossbar::creditArrived() {
  for(int i=0; i<numOutputs; i++) {
    if(creditsIn[i].event()) {
      int destinationBus = creditsIn[i].read().channelID() % numInputs; // is this right?

      assert(busReadyCredits[destinationBus][i/outputsPerComponent].read());
      creditsToBus[destinationBus][i/outputsPerComponent].write(creditsIn[i].read());
    }
  }
}

MulticastCrossbar::MulticastCrossbar(sc_module_name name, ComponentID ID, int inputs,
                                     int outputs, int outputsPerComponent,
                                     int channelsPerOutput, ChannelID startAddr,
                                     Dimension size, bool externalConnection) :
    Crossbar(name, ID, inputs, outputs, outputsPerComponent, channelsPerOutput,
             startAddr, size, externalConnection) {

  creditsIn          = new CreditInput[outputs];
  creditsOut         = new CreditOutput[inputs];

  ackCreditOut     = new ReadyInput[inputs];
  ackCreditIn  = new ReadyOutput[outputs];

  creditsToBus       = new sc_buffer<CreditType>*[numBuses];
  busReadyCredits    = new sc_signal<ReadyType>*[numBuses];

  for(int i=0; i<numBuses; i++) {
    creditsToBus[i]    = new sc_buffer<CreditType>[numMuxes];
    busReadyCredits[i] = new sc_signal<ReadyType>[numMuxes];
  }
}

MulticastCrossbar::~MulticastCrossbar() {
  delete[] creditsIn;
  delete[] creditsOut;
  delete[] ackCreditOut;
  delete[] ackCreditIn;

  for(int i=0; i<numBuses; i++) {
    delete[] creditsToBus[i];
    delete[] busReadyCredits[i];
  }

  delete[] creditsToBus;
  delete[] busReadyCredits;
}
