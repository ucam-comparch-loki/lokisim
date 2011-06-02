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

void LocalNetwork::makeArbiters() {
  // Numbers of input/output ports of arbiters. These may change depending on
  // the component they are arbitrating for, or whether they are dealing with
  // data or credits.
  int arbiterInputs;
  int dataArbiterOutputs;
  int creditArbiterOutputs;

  // Arbiters for cores.
  // Data can reach cores from any core, any memory, or from the global network.
  arbiterInputs = COMPONENTS_PER_TILE + 1;
  dataArbiterOutputs = CORE_INPUT_PORTS;
  creditArbiterOutputs = CORE_OUTPUT_PORTS;

  for(unsigned int i=0; i<CORES_PER_TILE; i++) {
    // Compute which ports the arbiter should connect to.
    int firstPortIndex = i*dataArbiterOutputs;
    makeDataArbiter(i, arbiterInputs, dataArbiterOutputs, firstPortIndex);

    // Compute which ports the arbiter should connect to.
    firstPortIndex = i*creditArbiterOutputs;
    makeCreditArbiter(i, numOutputs, creditArbiterOutputs, firstPortIndex);
  }

  // Arbiters for memories.
  // Data can only reach memories from cores. Memories communicate with each
  // other through a separate network, and with other tiles through a local
  // core.
  arbiterInputs = CORES_PER_TILE;
  dataArbiterOutputs = MEMORY_INPUT_PORTS;
  creditArbiterOutputs = MEMORY_OUTPUT_PORTS;

  for(unsigned int i=0; i<MEMS_PER_TILE; i++) {
    // Compute which ports the arbiter should connect to.
    int firstPortIndex = CORES_PER_TILE*CORE_INPUT_PORTS + i*dataArbiterOutputs;
    makeDataArbiter(CORES_PER_TILE + i, arbiterInputs, dataArbiterOutputs, firstPortIndex);

    // TODO: remove credit arbiter when new memory system is in place.
    // Compute which ports the arbiter should connect to.
    firstPortIndex = CORES_PER_TILE*CORE_OUTPUT_PORTS + i*creditArbiterOutputs;
    makeCreditArbiter(CORES_PER_TILE + i, numOutputs, creditArbiterOutputs, firstPortIndex);
  }

  // Arbiters for global network.
  // Data can only reach the global network from cores.
  arbiterInputs = CORES_PER_TILE;
  dataArbiterOutputs = 1;             // Make this a parameter?
  creditArbiterOutputs = 1;           // Make this a parameter?

  // Compute which ports the arbiter should connect to.
  int firstPortIndex = CORES_PER_TILE*CORE_INPUT_PORTS +
                       MEMS_PER_TILE*MEMORY_INPUT_PORTS;
  makeDataArbiter(COMPONENTS_PER_TILE, arbiterInputs, dataArbiterOutputs, firstPortIndex);

  // Compute which ports the arbiter should connect to.
  firstPortIndex = CORES_PER_TILE*CORE_OUTPUT_PORTS +
                   MEMS_PER_TILE*MEMORY_OUTPUT_PORTS;
  makeCreditArbiter(COMPONENTS_PER_TILE, numOutputs, creditArbiterOutputs, firstPortIndex);
}

void LocalNetwork::makeDataArbiter(unsigned int ID,
                                   unsigned int inputs,
                                   unsigned int outputs,
                                   unsigned int firstPort) {
  assert(firstPort + outputs - 1 < numOutputs);

  // Create an arbiter and store it in the vector.
  ArbiterComponent* arbiter =
      new ArbiterComponent(sc_gen_unique_name("data_arbiter"), ID, inputs, outputs);
  arbiters.push_back(arbiter);

  // Connect up the new arbiter.
  arbiter->clock(clock);

  for(unsigned int i=0; i<outputs; i++) {
    arbiter->dataOut[i](dataOut[firstPort + i]);
    arbiter->validDataOut[i](validDataOut[firstPort + i]);
    arbiter->ackDataOut[i](ackDataOut[firstPort + i]);
  }

  for(unsigned int i=0; i<inputs; i++) {
    arbiter->dataIn[i](dataSig[ID][i]);
    arbiter->validDataIn[i](validDataSig[ID][i]);
    arbiter->ackDataIn[i](ackDataSig[ID][i]);
  }
}

void LocalNetwork::makeCreditArbiter(unsigned int ID,
                                     unsigned int inputs,
                                     unsigned int outputs,
                                     unsigned int firstPort) {
  assert(firstPort + outputs - 1 < numInputs);

  // Create an arbiter and store it in the vector.
  ArbiterComponent* arbiter =
      new ArbiterComponent(sc_gen_unique_name("credit_arbiter"), ID, inputs, outputs);
  arbiters.push_back(arbiter);

  // Connect up the new arbiter.
  arbiter->clock(clock);

  for(unsigned int i=0; i<outputs; i++) {
    arbiter->dataOut[i](creditsOut[firstPort + i]);
    arbiter->validDataOut[i](validCreditOut[firstPort + i]);
    arbiter->ackDataOut[i](ackCreditOut[firstPort + i]);
  }

  for(unsigned int i=0; i<inputs; i++) {
    arbiter->dataIn[i](creditSig[ID][i]);
    arbiter->validDataIn[i](validCreditSig[ID][i]);
    arbiter->ackDataIn[i](ackCreditSig[ID][i]);
  }
}

void LocalNetwork::makeBuses() {
  ChannelID startAddr(id.getTile(), 0, 0);
  for(unsigned int i=0; i<numInputs; i++) {
    int busOutPorts, busOutChannels;
    if(i<CORES_PER_TILE*CORE_OUTPUT_PORTS) {
      busOutPorts = COMPONENTS_PER_TILE;
      busOutChannels = CORES_PER_TILE*CORE_INPUT_CHANNELS +
                       MEMS_PER_TILE*MEMORY_INPUT_CHANNELS;
    }
    else {
      busOutPorts = CORES_PER_TILE;
      busOutChannels = CORES_PER_TILE*CORE_INPUT_CHANNELS;
    }

    Bus* bus = new Bus(sc_gen_unique_name("data_bus"), i, busOutPorts,
                       Network::COMPONENT, size);
    buses.push_back(bus);

    bus->clock(clock);

    bus->dataIn[0](dataIn[i]);
    bus->validDataIn[0](validDataIn[i]);
    bus->ackDataIn[0](ackDataIn[i]);

    for(int j=0; j<busOutPorts; j++) {
      bus->dataOut[j](dataSig[j][i]);
      bus->validDataOut[j](validDataSig[j][i]);
      bus->ackDataOut[j](ackDataSig[j][i]);
    }
  }

  // TODO: won't need a bus for memory inputs.
  // Would it be better to have fewer credit buses so there are the same number
  // as data outputs?
  for(unsigned int i=0; i<numOutputs; i++) {
    int busOutPorts, busOutChannels;
    if(i<CORES_PER_TILE*CORE_INPUT_PORTS) {
      busOutPorts = COMPONENTS_PER_TILE;
      busOutChannels = CORES_PER_TILE*CORE_OUTPUT_CHANNELS +
                       MEMS_PER_TILE*MEMORY_OUTPUT_CHANNELS;
    }
    else {
      busOutPorts = CORES_PER_TILE;
      busOutChannels = CORES_PER_TILE*CORE_OUTPUT_CHANNELS;
    }

    Bus* bus = new Bus(sc_gen_unique_name("credit_bus"), i, busOutPorts,
                       Network::COMPONENT, size);
    buses.push_back(bus);

    bus->clock(clock);

    bus->dataIn[0](creditsIn[i]);
    bus->validDataIn[0](validCreditIn[i]);
    bus->ackDataIn[0](ackCreditIn[i]);

    for(int j=0; j<busOutPorts; j++) {
      bus->dataOut[j](creditSig[j][i]);
      bus->validDataOut[j](validCreditSig[j][i]);
      bus->ackDataOut[j](ackCreditSig[j][i]);
    }
  }
}

void LocalNetwork::makeWires() {
  dataSig        = new DataSignal*[COMPONENTS_PER_TILE + 1];
  validDataSig   = new ReadySignal*[COMPONENTS_PER_TILE + 1];
  ackDataSig     = new ReadySignal*[COMPONENTS_PER_TILE + 1];

  // TODO: don't need credit signals to memories with new memory system.
  creditSig      = new CreditSignal*[COMPONENTS_PER_TILE + 1];
  validCreditSig = new ReadySignal*[COMPONENTS_PER_TILE + 1];
  ackCreditSig   = new ReadySignal*[COMPONENTS_PER_TILE + 1];

  for(unsigned int i=0; i<COMPONENTS_PER_TILE+1; i++) {
    int numPorts;

    // Any component (or the global network) can send to cores.
    if(i<CORES_PER_TILE) numPorts = COMPONENTS_PER_TILE + 1;
    // Only cores can send to memories or the global network.
    else numPorts = CORES_PER_TILE;

    dataSig[i]        = new DataSignal[numPorts];
    validDataSig[i]   = new ReadySignal[numPorts];
    ackDataSig[i]     = new ReadySignal[numPorts];

    creditSig[i]      = new CreditSignal[numOutputs];
    validCreditSig[i] = new ReadySignal[numOutputs];
    ackCreditSig[i]   = new ReadySignal[numOutputs];
  }
}

CreditInput&  LocalNetwork::externalCreditIn() const {return creditsIn[numOutputs-1];}
CreditOutput& LocalNetwork::externalCreditOut() const {return creditsOut[numInputs-1];}
ReadyInput&   LocalNetwork::externalValidCreditIn() const {return validCreditIn[numOutputs-1];}
ReadyOutput&  LocalNetwork::externalValidCreditOut() const {return validCreditOut[numInputs-1];}
ReadyInput&   LocalNetwork::externalAckCreditIn() const {return ackCreditOut[numInputs-1];}
ReadyOutput&  LocalNetwork::externalAckCreditOut() const {return ackCreditIn[numOutputs-1];}

LocalNetwork::LocalNetwork(sc_module_name name, ComponentID tile) :
    Network(name,
            tile,
            OUTPUT_PORTS_PER_TILE,  // Inputs to this network
            INPUT_PORTS_PER_TILE,   // Outputs from this network
            Network::COMPONENT,     // This network connects components (not tiles, etc)
            Dimension(1.0, 0.2),    // Size in mm
            true) {                 // Add an extra input/output for global network

  // TODO: don't send/receive credits from memories when new memory system is
  //       in place.
  creditsIn      = new CreditInput[numOutputs];
  validCreditIn  = new ReadyInput[numOutputs];
  ackCreditIn    = new ReadyOutput[numOutputs];

  creditsOut     = new CreditOutput[numInputs];
  validCreditOut = new ReadyOutput[numInputs];
  ackCreditOut   = new ReadyInput[numInputs];

  makeWires();
  makeArbiters();
  makeBuses();
}

LocalNetwork::~LocalNetwork() {
  delete[] creditsIn;             delete[] creditsOut;
  delete[] validCreditIn;         delete[] validCreditOut;
  delete[] ackCreditIn;           delete[] ackCreditOut;

  for(unsigned int i=0; i<arbiters.size(); i++) delete arbiters[i];
  for(unsigned int i=0; i<buses.size(); i++) delete buses[i];

  for(unsigned int i=0; i<COMPONENTS_PER_TILE+1; i++) {
    delete[] dataSig[i];          delete[] creditSig[i];
    delete[] validDataSig[i];     delete[] validCreditSig[i];
    delete[] ackDataSig[i];       delete[] ackCreditSig[i];
  }

  delete[] dataSig;               delete[] creditSig;
  delete[] validDataSig;          delete[] validCreditSig;
  delete[] ackDataSig;            delete[] ackCreditSig;
}
