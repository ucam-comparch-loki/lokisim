/*
 * NewCrossbar.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "NewCrossbar.h"
#include "../Multiplexer.h"
#include "../Arbiters/EndArbiter.h"
#include "../../Utility/Instrumentation/Network.h"

void NewCrossbar::inputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarInput(oldInputs[port], dataIn[port].read(), port);
  oldInputs[port] = dataIn[port].read();
}

void NewCrossbar::outputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarOutput(oldOutputs[port], dataOut[port].read());
  oldOutputs[port] = dataOut[port].read();
}

void NewCrossbar::makePorts() {
  requestsIn.init(numInputPorts(), numArbiters);
  grantsOut.init(numInputPorts(), numArbiters);
  readyIn.init(numArbiters, buffersPerComponent);
}

void NewCrossbar::makeSignals() {
  selectSig.init(numArbiters, outputsPerComponent);
}

void NewCrossbar::makeArbiters() {
  for (int i=0; i<numArbiters; i++) {
    EndArbiter* arb;
    arb = new EndArbiter(sc_gen_unique_name("arbiter"), i,
                         numInputPorts(), outputsPerComponent, true, buffersPerComponent);

    arb->clock(clock);

    for (int j=0; j<buffersPerComponent; j++)
      arb->readyIn[j](readyIn[i][j]);
    for (uint j=0; j<numInputPorts(); j++) {
      arb->requests[j](requestsIn[j][i]);
      arb->grants[j](grantsOut[j][i]);
    }
    for (int j=0; j<outputsPerComponent; j++)
      arb->select[j](selectSig[i][j]);

    arbiters.push_back(arb);
  }
}

void NewCrossbar::makeMuxes() {
  for (int i=0; i<numMuxes; i++) {
    Multiplexer* mux = new Multiplexer(sc_gen_unique_name("mux"), numInputPorts());

    mux->dataOut(dataOut[i]);

    for (uint j=0; j<numInputPorts(); j++)
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
                         int buffersPerComponent) :
    NewNetwork(name, ID, inputs, outputs, level),
    numArbiters(outputs/outputsPerComponent),
    numMuxes(outputs),
    outputsPerComponent(outputsPerComponent),
    buffersPerComponent(buffersPerComponent),
    oldInputs(numInputPorts()),
    oldOutputs(numOutputPorts()) {

  makePorts();
  makeSignals();
  makeArbiters();
  makeMuxes();

  for (uint i=0; i<numInputPorts(); i++)
    SPAWN_METHOD(dataIn[i], NewCrossbar::inputChanged, i, false);
  for (uint i=0; i<numOutputPorts(); i++)
    SPAWN_METHOD(dataOut[i], NewCrossbar::outputChanged, i, false);
}

NewCrossbar::~NewCrossbar() {
  for (unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for (unsigned int i=0; i<muxes.size();    i++) delete muxes[i];
}
