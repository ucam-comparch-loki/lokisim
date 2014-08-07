/*
 * NewCrossbar.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Crossbar.h"
#include "../Multiplexer.h"
#include "../Arbiters/EndArbiter.h"
#include "../../Utility/Instrumentation/Network.h"

void Crossbar::inputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarInput(oldInputs[port], iData[port].read(), port);
  oldInputs[port] = iData[port].read();
//  cout << this->name() << " received " << dataIn[port].read() << endl;
}

void Crossbar::outputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarOutput(oldOutputs[port], oData[port].read());
  oldOutputs[port] = oData[port].read();
//  cout << this->name() << " sent " << dataOut[port].read() << endl;
}

void Crossbar::makePorts() {
  iRequest.init(numInputPorts(), numArbiters);
  oGrant.init(numInputPorts(), numArbiters);
  iReady.init(numArbiters, buffersPerComponent);
}

void Crossbar::makeSignals() {
  selectSig.init(numArbiters, outputsPerComponent);
}

void Crossbar::makeArbiters() {
  for (int i=0; i<numArbiters; i++) {
    EndArbiter* arb;
    arb = new EndArbiter(sc_gen_unique_name("arbiter"), i, numInputPorts(),
                         outputsPerComponent, true, buffersPerComponent);

    arb->clock(clock);

    for (int j=0; j<buffersPerComponent; j++)
      arb->iReady[j](iReady[i][j]);
    for (uint j=0; j<numInputPorts(); j++) {
      arb->iRequest[j](iRequest[j][i]);
      arb->oGrant[j](oGrant[j][i]);
    }
    for (int j=0; j<outputsPerComponent; j++)
      arb->oSelect[j](selectSig[i][j]);

    arbiters.push_back(arb);
  }
}

void Crossbar::makeMuxes() {
  for (int i=0; i<numMuxes; i++) {
    Multiplexer* mux = new Multiplexer(sc_gen_unique_name("mux"), numInputPorts());

    mux->oData(oData[i]);

    for (uint j=0; j<numInputPorts(); j++)
      mux->iData[j](iData[j]);

    int arbiter = i/outputsPerComponent;
    int arbiterSelect = i%outputsPerComponent;
    mux->iSelect(selectSig[arbiter][arbiterSelect]);

    muxes.push_back(mux);
  }
}

Crossbar::Crossbar(const sc_module_name& name,
                   const ComponentID& ID,
                   int inputs,
                   int outputs,
                   int outputsPerComponent,
                   HierarchyLevel level,
                   int buffersPerComponent) :
    Network(name, ID, inputs, outputs, level),
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
    SPAWN_METHOD(iData[i], Crossbar::inputChanged, i, false);
  for (uint i=0; i<numOutputPorts(); i++)
    SPAWN_METHOD(oData[i], Crossbar::outputChanged, i, false);
}

Crossbar::~Crossbar() {
  for (unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for (unsigned int i=0; i<muxes.size();    i++) delete muxes[i];
}
