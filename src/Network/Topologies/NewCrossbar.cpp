/*
 * NewCrossbar.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#include "NewCrossbar.h"
#include "../Multiplexer.h"
#include "../Arbiters/EndArbiter.h"

void NewCrossbar::makePorts() {
  requestsIn.init(numInputPorts(), numArbiters);
  grantsOut.init(numInputPorts(), numArbiters);
  readyIn.init(numArbiters, buffersPerComponent);
}

void NewCrossbar::makeSignals() {
  selectSig.init(numArbiters, outputsPerComponent);
}

void NewCrossbar::makeArbiters() {
  for(int i=0; i<numArbiters; i++) {
    EndArbiter* arb;
    arb = new EndArbiter(sc_gen_unique_name("arbiter"), i,
                         numInputPorts(), outputsPerComponent, true, buffersPerComponent);

    arb->clock(clock);

    for(int j=0; j<buffersPerComponent; j++)
      arb->readyIn[j](readyIn[i][j]);
    for(uint j=0; j<numInputPorts(); j++) {
      arb->requests[j](requestsIn[j][i]);
      arb->grants[j](grantsOut[j][i]);
    }
    for(int j=0; j<outputsPerComponent; j++)
      arb->select[j](selectSig[i][j]);

    arbiters.push_back(arb);
  }
}

void NewCrossbar::makeMuxes() {
  for(int i=0; i<numMuxes; i++) {
    Multiplexer* mux = new Multiplexer(sc_gen_unique_name("mux"), numInputPorts());

    mux->dataOut(dataOut[i]);

    for(uint j=0; j<numInputPorts(); j++)
      mux->dataIn[j](dataIn[j]);

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
    numMuxes(outputs),
    outputsPerComponent(outputsPerComponent),
    buffersPerComponent(buffersPerComponent) {

  makePorts();
  makeSignals();
  makeArbiters();
  makeMuxes();

}

NewCrossbar::~NewCrossbar() {
  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for(unsigned int i=0; i<muxes.size();    i++) delete muxes[i];
}
