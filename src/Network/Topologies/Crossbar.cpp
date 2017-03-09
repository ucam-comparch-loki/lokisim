/*
 * NewCrossbar.cpp
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Crossbar.h"
#include "../Multiplexer.h"
#include "../Arbiters/ClockedArbiter.h"
#include "../../Utility/Instrumentation/Network.h"

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
  iData.init(inputs, "iData");
  oData.init(outputs, "oData");
  iRequest.init(inputs, numArbiters, "iRequest");
  oGrant.init(inputs, numArbiters, "oGrant");
  iReady.init(numArbiters, buffersPerComponent, "iReady");
}

void Crossbar::makeSignals() {
  selectSig.init(numArbiters, outputsPerComponent, "selectSig");
}

void Crossbar::makeArbiters() {
  for (int i=0; i<numArbiters; i++) {
    ClockedArbiter* arb;
    arb = new ClockedArbiter(sc_gen_unique_name("arbiter"), i, iData.size(),
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
        os << this->name() << " unable to send to " << ChannelID(id.tile.x, id.tile.y, component, buffer) << endl;
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

Crossbar::~Crossbar() {
  for (unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for (unsigned int i=0; i<muxes.size();    i++) delete muxes[i];
}
