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
                                         i, numMuxes, level, size);
    buses.push_back(bus);

    bus->dataIn[0](dataIn[i]);

//    bus->creditsOut[0](creditsOut[i]);

    for(int j=0; j<numMuxes; j++) {
      bus->dataOut[j](busToMux[i][j]);

//      bus->creditsIn[j](creditsToBus[i][j]);
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

// Sort out arguments so there are fewer/they make more sense?
MulticastCrossbar::MulticastCrossbar(const sc_module_name& name,
                                     const ComponentID& ID,
                                     int inputs,
                                     int outputs,
                                     int outputsPerComponent,
                                     int inputsPerComponent,
                                     HierarchyLevel level,
                                     Dimension size,
                                     bool externalConnection) :
    Crossbar(name, ID, inputs, outputs, outputsPerComponent,
             level, size, externalConnection) {

  creditsIn        = new CreditInput[numOutputs];
  creditsOut       = new CreditOutput[numInputs];

  creditsToBus     = new CreditSignal*[numBuses];

  for(int i=0; i<numBuses; i++)
    creditsToBus[i]     = new CreditSignal[numMuxes];

  // Create an UnclockedNetwork containing an ordinary crossbar to carry
  // credits to the appropriate buses. (Some form of multi-input demux would be
  // preferable, but a small crossbar behaves in the same way, and doesn't
  // require additional implementation effort.)
  for(int i=0; i<numMuxes; i++) {
    // Create the crossbar.
    Crossbar* c = new Crossbar(sc_gen_unique_name("credit"), i,
                               outputsPerComponent, numBuses, inputsPerComponent,
                               level, size, false);
    c->initialise();
    UnclockedNetwork* n = new UnclockedNetwork(sc_gen_unique_name("wrapper"),c);
    creditCrossbars.push_back(n);

    // Wire the new crossbar up.
    for(int j=0; j<outputsPerComponent; j++)
      n->dataIn[j](creditsIn[i*outputsPerComponent + j]);

    for(int j=0; j<numBuses; j++)
      n->dataOut[j](creditsToBus[j][i]);
  }

}

MulticastCrossbar::~MulticastCrossbar() {
  delete[] creditsIn;
  delete[] creditsOut;

  for(int i=0; i<numBuses; i++)
    delete[] creditsToBus[i];

  delete[] creditsToBus;

  for(unsigned int i=0; i<creditCrossbars.size(); i++) delete creditCrossbars[i];
}
