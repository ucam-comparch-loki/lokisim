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
  requestsIn.init(numBuses, numArbiters);
  grantsOut.init(numBuses, numArbiters);
  readyIn.init(numArbiters, buffersPerComponent);
}

void NewCrossbar::makeSignals() {
  dataSig.init(numBuses);
  selectSig.init(numArbiters, outputsPerComponent);
}

void NewCrossbar::makeArbiters() {
  for(int i=0; i<numArbiters; i++) {
    EndArbiter* arb;
    arb = new EndArbiter(sc_gen_unique_name("arbiter"), i,
                         numBuses, outputsPerComponent, true, buffersPerComponent);

    arb->clock(clock);

    for(int j=0; j<buffersPerComponent; j++)
      arb->readyIn[j](readyIn[i][j]);
    for(int j=0; j<numBuses; j++) {
      arb->requests[j](requestsIn[j][i]);
      arb->grants[j](grantsOut[j][i]);
    }
    for(int j=0; j<outputsPerComponent; j++)
      arb->select[j](selectSig[i][j]);

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
                         int buffersPerComponent) :
    NewNetwork(name, ID, inputs, outputs, level, size),
    numArbiters(outputs/outputsPerComponent),
    numBuses(inputs),
    numMuxes(outputs),
    outputsPerComponent(outputsPerComponent),
    buffersPerComponent(buffersPerComponent) {

  makePorts();
  makeSignals();
  makeArbiters();
  makeBuses();
  makeMuxes();

}

NewCrossbar::~NewCrossbar() {
  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for(unsigned int i=0; i<buses.size();    i++) delete buses[i];
  for(unsigned int i=0; i<muxes.size();    i++) delete muxes[i];
}
