/*
 * NewCrossbar.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Crossbar.h"
#include "../../Utility/Instrumentation/Network.h"

using sc_core::sc_gen_unique_name;
using sc_core::sc_module_name;

void Crossbar::inputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarInput(oldInputs[port], iData[port].read(), port);
  oldInputs[port] = iData[port].read();
//  cout << this->name() << " received " << iData[port].read() << endl;
}

void Crossbar::outputChanged(const PortIndex port) {
  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarOutput(oldOutputs[port], oData[port].read());
  oldOutputs[port] = oData[port].read();
//  cout << this->name() << " sent " << oData[port].read() << endl;
}

void Crossbar::makePorts(uint inputs, uint outputs) {
  iData.init("iData", inputs);
  oData.init("oData", outputs);
  iRequest.init("iRequest", inputs, numArbiters);
  oGrant.init("oGrant", inputs, numArbiters);
  iReady.init("iReady", numArbiters, buffersPerComponent);
}

void Crossbar::makeSignals() {
  selectSig.init("selectSig", numArbiters, outputsPerComponent);
}

void Crossbar::makeArbiters() {
  for (int i=0; i<numArbiters; i++) {
    ClockedArbiter* arb;
    arb = new ClockedArbiter(sc_gen_unique_name("arbiter"), iData.size(),
                         outputsPerComponent, true, buffersPerComponent);

    arb->clock(clock);
    arb->iReady(iReady[i]);

    for (uint j=0; j<iData.size(); j++) {
      arb->iRequest[j](iRequest[j][i]);
      arb->oGrant[j](oGrant[j][i]);
    }
    arb->oSelect(selectSig[i]);

    arbiters.push_back(arb);
  }
}

void Crossbar::makeMuxes() {
  for (int i=0; i<numMuxes; i++) {
    Multiplexer* mux = new Multiplexer(sc_gen_unique_name("mux"), iData.size());

    mux->oData(oData[i]);
    mux->iData(iData);

    int arbiter = i/outputsPerComponent;
    int arbiterSelect = i%outputsPerComponent;
    mux->iSelect(selectSig[arbiter][arbiterSelect]);

    muxes.push_back(mux);
  }
}

void Crossbar::reportStalls(ostream& os) {
  for (unsigned int i=0; i<iData.size(); i++)
    if (iData[i].valid())
      os << this->name() << " has data on input port " << i << ": " << iData[i].read() << endl;
  for (unsigned int i=0; i<oData.size(); i++)
    if (oData[i].valid())
      os << this->name() << " has data on output port " << i << ": " << oData[i].read() << endl;
  for (unsigned int component=0; component<iReady.size(); component++)
    for (unsigned int buffer=0; buffer<iReady[component].size(); buffer++)
      if (!iReady[component][buffer])
        os << this->name() << " unable to send to component " << component << ", channel " << buffer << endl;
}

Crossbar::Crossbar(const sc_module_name& name,
                   int inputs,
                   int outputs,
                   int outputsPerComponent,
                   int buffersPerComponent) :
    Network(name),
    clock("clock"),
    numArbiters(outputs/outputsPerComponent),
    numMuxes(outputs),
    outputsPerComponent(outputsPerComponent),
    buffersPerComponent(buffersPerComponent),
    oldInputs(inputs),
    oldOutputs(outputs) {

  makePorts(inputs, outputs);
  makeSignals();
  makeArbiters();
  makeMuxes();

  for (uint i=0; i<iData.size(); i++)
    SPAWN_METHOD(iData[i], Crossbar::inputChanged, i, false);
  for (uint i=0; i<oData.size(); i++)
    SPAWN_METHOD(oData[i], Crossbar::outputChanged, i, false);
}
