/*
 * LocalNetwork.cpp
 *
 *  Created on: 9 May 2011
 *      Author: db434
 */

#include "LocalNetwork.h"
#include "Bus.h"
#include "../Arbiters/ArbiterComponent.h"
#include "../../TileComponents/TileComponent.h"

// Only cores and the global network can send/receive credits.
const unsigned int LocalNetwork::creditInputs  = CORES_PER_TILE + 1;
const unsigned int LocalNetwork::creditOutputs = CORES_PER_TILE * CORE_OUTPUT_PORTS + 1;

void LocalNetwork::makeArbiters() {
  // Arguments for ArbiterComponent constructor:
  //   name, ID, number of inputs, number of outputs, whether to use wormhole routing.

  // Keep track of how many ports have been bound, so we know which to bind next.
  PortIndex port = 0;
  ArbiterComponent* arbiter;

  // Arbiters for data entering a core.
  // Data can arrive from any core, any memory, or the global network.
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    arbiter = new ArbiterComponent(sc_gen_unique_name("core_data_arb"), i,
                                   COMPONENTS_PER_TILE+1, CORE_INPUT_PORTS, false);
    cDataArbiters.push_back(arbiter);
    bindArbiter(arbiter, port, true);
    port += arbiter->numOutputs();
  }

  // Arbiters for data entering a memory.
  // Data can arrive from any core.
  for(unsigned int i=0; i<MEMS_PER_TILE; i++) {
    arbiter = new ArbiterComponent(sc_gen_unique_name("mem_data_arb"), i,
                                   CORES_PER_TILE, MEMORY_INPUT_PORTS, true);
    mDataArbiters.push_back(arbiter);
    bindArbiter(arbiter, port, true);
    port += arbiter->numOutputs();
  }

  // Arbiters for data heading to the global network.
  // Data can arrive from any core.
  arbiter = new ArbiterComponent(sc_gen_unique_name("global_data_arb"), 0,
                                 CORES_PER_TILE, 1, false);
  gDataArbiters.push_back(arbiter);
  bindArbiter(arbiter, port, true);

  // Reset the port counter because we are now moving on to credit ports.
  port = 0;

  // Arbiters for credits entering a core.
  // Credits can only arrive from the global network.
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    arbiter = new ArbiterComponent(sc_gen_unique_name("core_credit_arb"), i,
                                   1, CORE_OUTPUT_PORTS, false);
    cCreditArbiters.push_back(arbiter);
    bindArbiter(arbiter, port, false);
    port += arbiter->numOutputs();
  }

  // Arbiters for credits heading to the global network.
  // Credits can arrive from any core.
  arbiter = new ArbiterComponent(sc_gen_unique_name("global_credit_arb"), 0,
                                 CORES_PER_TILE, 1, false);
  gCreditArbiters.push_back(arbiter);
  bindArbiter(arbiter, port, false);

}

void LocalNetwork::bindArbiter(ArbiterComponent* arbiter, PortIndex port, bool data) {
  arbiter->clock(clock);

  if(data) {
    // Connect to data outputs.
    for(unsigned int i=0; i<arbiter->numOutputs(); i++) {
      arbiter->dataOut[i](dataOut[port+i]);
      arbiter->validDataOut[i](validDataOut[port+i]);
      arbiter->ackDataOut[i](ackDataOut[port+i]);
    }
  }
  else {
    // Connect to credit outputs.
    for(unsigned int i=0; i<arbiter->numOutputs(); i++) {
      arbiter->dataOut[i](creditsOut[port+i]);
      arbiter->validDataOut[i](validCreditOut[port+i]);
      arbiter->ackDataOut[i](ackCreditOut[port+i]);
    }
  }
}

void LocalNetwork::makeBuses() {
  // Arguments for Bus constructor:
  //   name, ID, number of outputs, position in hierarchy, size, first addressable output.

  // Keep track of how many input ports we have bound so far.
  PortIndex port = 0;
  Bus* bus;

  // Data buses from cores.
  // Data can be sent to cores, memories, or the global network.
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {

    // Make a separate bus for each output port of the core.
    for(unsigned int j=0; j<CORE_OUTPUT_PORTS; j++) {

      switch(j) {
        case 0: { // To cores
          bus = new Bus(sc_gen_unique_name("c2c_data_bus"), i, CORES_PER_TILE,
                        Network::COMPONENT, size, 0);
          c2cDataBuses.push_back(bus);
          break;
        }
        case 1: { // To memories
          bus = new Bus(sc_gen_unique_name("c2m_data_bus"), i, MEMS_PER_TILE,
                        Network::COMPONENT, size, CORES_PER_TILE);
          c2mDataBuses.push_back(bus);
          break;
        }
        case 2: { // To global network
          bus = new Bus(sc_gen_unique_name("c2g_data_bus"), i, 1,
                        Network::NONE, size, 0);
          c2gDataBuses.push_back(bus);
          break;
        }
        default: assert(false);
      }

      bindBus(bus, port, true);
      port++;
    }
  }

  // Data buses from memories.
  // Data can be sent to cores.
  for(unsigned int i=0; i<MEMS_PER_TILE; i++) {
    assert(MEMORY_OUTPUT_PORTS == 1);

    bus = new Bus(sc_gen_unique_name("m2c_data_bus"), i, CORES_PER_TILE,
                  Network::COMPONENT, size, 0);
    m2cDataBuses.push_back(bus);
    bindBus(bus, port, true);
    port++;
  }

  // Data buses from the global network.
  // Data can be sent to cores.
  bus = new Bus(sc_gen_unique_name("g2c_data_bus"), 0, CORES_PER_TILE,
                Network::COMPONENT, size, 0);
  g2cDataBuses.push_back(bus);
  bindBus(bus, port, true);

  // Reset the port counter now that we are moving on to credits.
  port = 0;

  // Credit buses from cores.
  // Credits can only be sent to the global network.
  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    // Cores only have one credit output, which sends its credits to the global
    // network.
    bus = new Bus(sc_gen_unique_name("c2g_credit_bus"), i, 1,
                  Network::NONE, size, 0);
    c2gCreditBuses.push_back(bus);
    bindBus(bus, port, false);
    port++;
  }

  // Credit buses from the global network.
  // Credits can be sent to cores.
  bus = new Bus(sc_gen_unique_name("g2c_credit_bus"), 0, CORES_PER_TILE,
                Network::COMPONENT, size, 0);
  g2cCreditBuses.push_back(bus);
  bindBus(bus, port, false);

}

void LocalNetwork::bindBus(Bus* bus, PortIndex port, bool data) {
  bus->clock(clock);

  if(data) {
    bus->dataIn[0](dataIn[port]);
    bus->validDataIn[0](validDataIn[port]);
    bus->ackDataIn[0](ackDataIn[port]);
  }
  else {
    bus->dataIn[0](creditsIn[port]);
    bus->validDataIn[0](validCreditIn[port]);
    bus->ackDataIn[0](ackCreditIn[port]);
  }
}

void LocalNetwork::wireUp() {
  // Core-to-core sub-network.
  connect(c2cDataBuses, cDataArbiters, 0, true);
//  connect(c2cCreditBuses, cCreditArbiters, 0, false);

  // Core-to-memory sub-network.
  connect(c2mDataBuses, mDataArbiters, 0, true);
  connect(m2cDataBuses, cDataArbiters, CORES_PER_TILE, true);

  // Core-to-global-network sub-network.
  connect(c2gDataBuses, gDataArbiters, 0, true);
  connect(g2cCreditBuses, cCreditArbiters, 0, false);
  connect(g2cDataBuses, cDataArbiters, COMPONENTS_PER_TILE, true);
  connect(c2gCreditBuses, gCreditArbiters, 0, false);
}

void LocalNetwork::connect(vector<Bus*>& buses,
                           vector<ArbiterComponent*>& arbiters,
                           PortIndex firstArbiterConnection,
                           bool data) {

  if(buses.empty() || arbiters.empty()) return;

  assert(buses.size() <= arbiters[0]->numInputs());
  assert(buses[0]->numOutputPorts() == arbiters.size());

  // Connect the jth output of bus i to the ith input of arbiter j.
  for(unsigned int i=0; i<buses.size(); i++) {
    Bus* bus = buses[i];

    for(unsigned int j=0; j<arbiters.size(); j++) {
      ArbiterComponent* arb = arbiters[j];

      if(data) {
        // Create the wires needed to connect this bus to this arbiter.
        makeDataSigs();

        bus->dataOut[j](*dataSigs.back());
        bus->validDataOut[j](*validDataSigs.back());
        bus->ackDataOut[j](*ackDataSigs.back());
        arb->dataIn[firstArbiterConnection + i](*dataSigs.back());
        arb->validDataIn[firstArbiterConnection + i](*validDataSigs.back());
        arb->ackDataIn[firstArbiterConnection + i](*ackDataSigs.back());
      }
      else {
        // Create the wires needed to connect this bus to this arbiter.
        makeCreditSigs();

        bus->dataOut[j](*creditSigs.back());
        bus->validDataOut[j](*validCreditSigs.back());
        bus->ackDataOut[j](*ackCreditSigs.back());
        arb->dataIn[firstArbiterConnection + i](*creditSigs.back());
        arb->validDataIn[firstArbiterConnection + i](*validCreditSigs.back());
        arb->ackDataIn[firstArbiterConnection + i](*ackCreditSigs.back());
      }
    }
  }
}

void LocalNetwork::makeDataSigs() {
  // Make a set of data/valid/acknowledge signals, and append them to the vectors.
  dataSigs.push_back(new DataSignal());
  validDataSigs.push_back(new ReadySignal());
  ackDataSigs.push_back(new ReadySignal());
}

void LocalNetwork::makeCreditSigs() {
  // Make a set of credit/valid/acknowledge signals, and append them to the vectors.
  creditSigs.push_back(new CreditSignal());
  validCreditSigs.push_back(new ReadySignal());
  ackCreditSigs.push_back(new ReadySignal());
}

CreditInput&  LocalNetwork::externalCreditIn() const       {return creditsIn[creditInputs-1];}
CreditOutput& LocalNetwork::externalCreditOut() const      {return creditsOut[creditOutputs-1];}
ReadyInput&   LocalNetwork::externalValidCreditIn() const  {return validCreditIn[creditInputs-1];}
ReadyOutput&  LocalNetwork::externalValidCreditOut() const {return validCreditOut[creditOutputs-1];}
ReadyInput&   LocalNetwork::externalAckCreditIn() const    {return ackCreditOut[creditOutputs-1];}
ReadyOutput&  LocalNetwork::externalAckCreditOut() const   {return ackCreditIn[creditInputs-1];}

LocalNetwork::LocalNetwork(sc_module_name name, ComponentID tile) :
    Network(name,
            tile,
            OUTPUT_PORTS_PER_TILE,  // Inputs to this network
            INPUT_PORTS_PER_TILE,   // Outputs from this network
            Network::COMPONENT,     // This network connects components (not tiles, etc)
            Dimension(1.0, 0.2),    // Size in mm
            0,                      // First accessible component has ID of 0
            true) {                 // Add an extra input/output for global network

  creditsIn      = new CreditInput[creditInputs];
  validCreditIn  = new ReadyInput[creditInputs];
  ackCreditIn    = new ReadyOutput[creditInputs];

  creditsOut     = new CreditOutput[creditOutputs];
  validCreditOut = new ReadyOutput[creditOutputs];
  ackCreditOut   = new ReadyInput[creditOutputs];

  makeArbiters();
  makeBuses();
  wireUp();
}

LocalNetwork::~LocalNetwork() {
  delete[] creditsIn;             delete[] creditsOut;
  delete[] validCreditIn;         delete[] validCreditOut;
  delete[] ackCreditIn;           delete[] ackCreditOut;

  for(unsigned int i=0; i<c2cDataBuses.size();    i++) delete c2cDataBuses[i];
  for(unsigned int i=0; i<c2mDataBuses.size();    i++) delete c2mDataBuses[i];
  for(unsigned int i=0; i<c2gDataBuses.size();    i++) delete c2gDataBuses[i];
  for(unsigned int i=0; i<m2cDataBuses.size();    i++) delete m2cDataBuses[i];
  for(unsigned int i=0; i<g2cDataBuses.size();    i++) delete g2cDataBuses[i];
  for(unsigned int i=0; i<c2cCreditBuses.size();  i++) delete c2cCreditBuses[i];
  for(unsigned int i=0; i<c2gCreditBuses.size();  i++) delete c2gCreditBuses[i];
  for(unsigned int i=0; i<g2cCreditBuses.size();  i++) delete g2cCreditBuses[i];
  for(unsigned int i=0; i<cDataArbiters.size();   i++) delete cDataArbiters[i];
  for(unsigned int i=0; i<mDataArbiters.size();   i++) delete mDataArbiters[i];
  for(unsigned int i=0; i<gDataArbiters.size();   i++) delete gDataArbiters[i];
  for(unsigned int i=0; i<cCreditArbiters.size(); i++) delete cCreditArbiters[i];
  for(unsigned int i=0; i<gCreditArbiters.size(); i++) delete gCreditArbiters[i];

  for(unsigned int i=0; i<dataSigs.size();        i++) delete dataSigs[i];
  for(unsigned int i=0; i<validDataSigs.size();   i++) delete validDataSigs[i];
  for(unsigned int i=0; i<ackDataSigs.size();     i++) delete ackDataSigs[i];
  for(unsigned int i=0; i<creditSigs.size();      i++) delete creditSigs[i];
  for(unsigned int i=0; i<validCreditSigs.size(); i++) delete validCreditSigs[i];
  for(unsigned int i=0; i<ackCreditSigs.size();   i++) delete ackCreditSigs[i];
}
