/*
 * MulticastCrossbar.cpp
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#include "MulticastCrossbar.h"
#include "MulticastBus.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../UnclockedNetwork.h"
#include "../../Datatype/AddressedWord.h"

void MulticastCrossbar::makeBuses() {
  // Generate and connect up buses.
  for(int i=0; i<numBuses; i++) {
    MulticastBus* bus = new MulticastBus(sc_gen_unique_name("bus"),
                                         i, numMuxes, /*numOutputs*channelsPerOutput,
                                         firstOutput,*/ level, size);
    buses.push_back(bus);

    bus->dataIn[0](dataIn[i]);
    bus->validDataIn[0](validDataIn[i]);
    bus->ackDataIn[0](ackDataIn[i]);

//    bus->creditsOut(creditsOut[i]);
//    bus->validCreditOut(validCreditOut[i]);
//    bus->ackCreditOut(ackCreditOut[i]);

    for(int j=0; j<numMuxes; j++) {
      bus->dataOut[j](busToMux[i][j]);
      bus->validDataOut[j](newData[i][j]);
      bus->ackDataOut[j](readData[i][j]);

//      bus->creditsIn[j](creditsToBus[i][j]);
//      bus->validCreditIn[j](validCreditToBus[i][j]);
//      bus->ackCreditIn[j](ackCreditToBus[i][j]);
    }
  }
}

CreditInput& MulticastCrossbar::externalCreditIn() const {
  assert(externalConnection);
  return creditsIn[numOutputs-1];
}

CreditOutput& MulticastCrossbar::externalCreditOut() const {
  assert(externalConnection);
  return creditsOut[numInputs-1];
}

ReadyInput& MulticastCrossbar::externalValidCreditIn() const {
  assert(externalConnection);
  return validCreditIn[numOutputs-1];
}

ReadyOutput& MulticastCrossbar::externalValidCreditOut() const {
  assert(externalConnection);
  return validCreditOut[numInputs-1];
}

ReadyInput& MulticastCrossbar::externalAckCreditIn() const {
  assert(externalConnection);
  return ackCreditOut[numInputs-1];
}

ReadyOutput& MulticastCrossbar::externalAckCreditOut() const {
  assert(externalConnection);
  return ackCreditIn[numOutputs-1];
}

// Sort out arguments so there are fewer/they make more sense?
MulticastCrossbar::MulticastCrossbar(const sc_module_name& name,
                                     const ComponentID& ID,
                                     int inputs,
                                     int outputs,
                                     int outputsPerComponent,
//                                     int channelsPerOutput,
//                                     const ChannelID& firstOutput,
                                     int inputsPerComponent,
//                                     int channelsPerInput,
//                                     const ChannelID& firstInput,
                                     HierarchyLevel level,
                                     Dimension size,
                                     bool externalConnection) :
    Crossbar(name, ID, inputs, outputs, outputsPerComponent, /*channelsPerOutput,
             firstOutput,*/level, size, externalConnection) {

  creditsIn        = new CreditInput[numOutputs];
  validCreditIn    = new ReadyInput[numOutputs];
  ackCreditIn      = new ReadyOutput[numOutputs];

  creditsOut       = new CreditOutput[numInputs];
  validCreditOut   = new ReadyOutput[numInputs];
  ackCreditOut     = new ReadyInput[numInputs];

  creditsToBus     = new CreditSignal*[numBuses];
  validCreditToBus = new ReadySignal*[numBuses];
  ackCreditToBus   = new ReadySignal*[numBuses];

  for(int i=0; i<numBuses; i++) {
    creditsToBus[i]     = new CreditSignal[numMuxes];
    validCreditToBus[i] = new ReadySignal[numMuxes];
    ackCreditToBus[i]   = new ReadySignal[numMuxes];
  }

  // Create an UnclockedNetwork containing an ordinary crossbar to carry
  // credits to the appropriate buses. (Some form of multi-input demux would be
  // preferable, but a small crossbar behaves in the same way, and doesn't
  // require additional implementation effort.)
  for(int i=0; i<numMuxes; i++) {
    // Create the crossbar.
    Crossbar* c = new Crossbar(sc_gen_unique_name("credit"), i,
                               outputsPerComponent, numBuses, inputsPerComponent,
                               /*channelsPerInput, firstInput,*/level, size, false);
    c->initialise();
    UnclockedNetwork* n = new UnclockedNetwork(sc_gen_unique_name("wrapper"),c);
    creditCrossbars.push_back(n);

    // Wire the new crossbar up.
    for(int j=0; j<outputsPerComponent; j++) {
      n->dataIn[j](creditsIn[i*outputsPerComponent + j]);
      n->validDataIn[j](validCreditIn[i*outputsPerComponent + j]);
      n->ackDataIn[j](ackCreditIn[i*outputsPerComponent + j]);
    }

    for(int j=0; j<numBuses; j++) {
      n->dataOut[j](creditsToBus[j][i]);
      n->validDataOut[j](validCreditToBus[j][i]);
      n->ackDataOut[j](ackCreditToBus[j][i]);
    }
  }
}

MulticastCrossbar::~MulticastCrossbar() {
  delete[] creditsIn;
  delete[] validCreditIn;
  delete[] ackCreditIn;

  delete[] creditsOut;
  delete[] validCreditOut;
  delete[] ackCreditOut;

  for(int i=0; i<numBuses; i++) {
    delete[] creditsToBus[i];
    delete[] validCreditToBus[i];
    delete[] ackCreditToBus[i];
  }

  delete[] creditsToBus;
  delete[] validCreditToBus;
  delete[] ackCreditToBus;

  for(unsigned int i=0; i<creditCrossbars.size(); i++) delete creditCrossbars[i];
}
