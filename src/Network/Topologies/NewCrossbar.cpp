/*
 * NewCrossbar.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#include "NewCrossbar.h"
#include "NewBus.h"
#include "../Multiplexer.h"
#include "../Arbiters/ChainedArbiter.h"
#include "../Arbiters/EndArbiter.h"

void NewCrossbar::makePorts() {
  requestsIn = new RequestInput*[numBuses];
  grantsOut  = new GrantOutput*[numBuses];

  for(int i=0; i<numBuses; i++) {
    requestsIn[i] = new RequestInput[numArbiters];
    grantsOut[i]  = new GrantOutput[numArbiters];
  }

  if(chained) {
    requestsOut = new RequestOutput*[numArbiters];
    grantsIn    = new GrantInput*[numArbiters];

    for(int i=0; i<numArbiters; i++) {
      requestsOut[i] = new RequestOutput[outputsPerComponent];
      grantsIn[i]    = new GrantInput[outputsPerComponent];
    }
  }
  else {
    readyIn = new ReadyInput[numArbiters];
  }
}

void NewCrossbar::makeSignals() {
  dataSig   = new DataSignal[numBuses];
  selectSig = new SelectSignal*[numArbiters];

  for(int i=0; i<numArbiters; i++) {
    selectSig[i] = new SelectSignal[outputsPerComponent];
  }
}

void NewCrossbar::makeArbiters() {
  for(int i=0; i<numArbiters; i++) {
    BasicArbiter* arb;

    // Need a different type of arbiter depending on if more arbitration is
    // needed after this network.
    if(chained) {
      arb = new ChainedArbiter(sc_gen_unique_name("arbiter"), i,
                               numBuses, outputsPerComponent, true);

      // Connect up ports which are unique to this type of arbiter, allowing it
      // to issue an arbitration request to the next arbiter in the chain.
      for(int j=0; j<outputsPerComponent; j++) {
        (static_cast<ChainedArbiter*>(arb))->requestOut[j](requestsOut[i][j]);
        (static_cast<ChainedArbiter*>(arb))->grantIn[j](grantsIn[i][j]);
      }
    }
    else {
      arb = new EndArbiter(sc_gen_unique_name("arbiter"), i,
                           numBuses, outputsPerComponent, true);

      // Connect up ports which are unique to this type of arbiter.
      for(int j=0; j<outputsPerComponent; j++)
        (static_cast<EndArbiter*>(arb))->readyIn[j](readyIn[j + i*outputsPerComponent]);
    }

    // Connect up all ports which are common to the two types of arbiter.
    arb->clock(clock);

    for(int j=0; j<numBuses; j++) {
      arb->requests[j](requestsIn[j][i]);
      arb->grants[j](grantsOut[j][i]);
    }
    for(int j=0; j<outputsPerComponent; j++) {
      arb->select[j](selectSig[i][j]);
    }

    arbiters.push_back(arb);
  }
}

void NewCrossbar::makeBuses() {
  for(int i=0; i<numBuses; i++) {
    NewBus* bus = new NewBus(sc_gen_unique_name("bus"), i, numMuxes, size);

    bus->dataIn(dataIn[i]);
    bus->dataOut(dataSig[i]);

    buses.push_back(bus);
  }
}

void NewCrossbar::makeMuxes() {
  for(int i=0; i<numMuxes; i++) {
    Multiplexer* mux = new Multiplexer(sc_gen_unique_name("mux"), numBuses);

    mux->dataOut(dataOut[i]);

    for(int j=0; j<numBuses; j++)
      mux->dataIn[j](dataSig[j]);

    int arbiter = i/outputsPerComponent;
    int arbiterSelect = i%outputsPerComponent;
    mux->select(selectSig[arbiter][arbiterSelect]);

    muxes.push_back(mux);
  }
}

NewCrossbar::NewCrossbar(const sc_module_name& name,
                         const ComponentID& ID,
                         int inputs,
                         int outputs,
                         int outputsPerComponent,
                         HierarchyLevel level,
                         Dimension size,
                         bool chained) :
    NewNetwork(name, ID, inputs, outputs, level, size),
    numArbiters(outputs/outputsPerComponent),
    numBuses(inputs),
    numMuxes(outputs),
    outputsPerComponent(outputsPerComponent),
    chained(chained) {

  makePorts();
  makeSignals();
  makeArbiters();
  makeBuses();
  makeMuxes();

}

NewCrossbar::~NewCrossbar() {
  delete[] dataSig;

  for(int i=0; i<numArbiters; i++) delete[] selectSig[i];
  delete[] selectSig;

  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for(unsigned int i=0; i<buses.size();    i++) delete buses[i];
  for(unsigned int i=0; i<muxes.size();    i++) delete muxes[i];

  for(int i=0; i<numBuses; i++) {
    delete[] requestsIn[i];  delete[] grantsOut[i];
  }

  delete[] requestsIn;  delete[] grantsOut;

  if(chained) {
    for(int i=0; i<numArbiters; i++) {
      delete[] requestsOut[i]; delete[] grantsIn[i];
    }

    delete[] requestsOut; delete[] grantsIn;
  }
  else
    delete[] readyIn;
}
