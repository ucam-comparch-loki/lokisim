/*
 * ForwardingNetwork.cpp
 *
 *  Created on: Jan 31, 2020
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ForwardingNetwork.h"
#include "../../Utility/Assert.h"

namespace Compute {

ForwardingNetwork::ForwardingNetwork(sc_module_name name, size_t numRegisters) :
    LokiComponent(name),
    producers(numRegisters),
    newProducer(numRegisters),
    consumers(numRegisters) {

  // Set up methods to remove producers and consumers when appropriate.
  for (uint i=0; i<producers.size(); i++) {
    SPAWN_METHOD(newProducer[i], ForwardingNetwork::removeProducer, i, false);
    SPAWN_METHOD(newProducer[i], ForwardingNetwork::removeConsumer, i, false);
  }
}

void ForwardingNetwork::addProducer(DecodedInstruction producer) {
  // Also assert that the register written to is not a protected one, e.g. read-
  // only, but don't have access to that information here.
  if (!producer->writesRegister())
    return;

  RegisterIndex destination = producer->getDestinationRegister();

  loki_assert(!producers[destination]);
  producers[destination] = producer;
  newProducer[destination].notify(sc_core::SC_ZERO_TIME);
}

bool ForwardingNetwork::requiresForwarding(DecodedInstruction consumer,
                                           PortIndex port) const {
  if (!consumer->readsRegister(port))
    return false;

  RegisterIndex source = consumer->getSourceRegister(port);
  return (bool)producers[source];
}

void ForwardingNetwork::addConsumer(DecodedInstruction consumer,
                                    PortIndex port) {
  if (requiresForwarding(consumer, port)) {
    RegisterIndex source = consumer->getSourceRegister(port);

    consumer_t toStore(consumer, port);
    consumers[source].push_back(toStore);

    // Get the result immediately if available.
    if (producers[source]->completedPhase(InstructionInterface::INST_COMPUTE))
      resultProduced(source);
  }
}

void ForwardingNetwork::resultProduced(RegisterIndex reg) {
  loki_assert(producers[reg]->completedPhase(InstructionInterface::INST_COMPUTE));
  int32_t result = producers[reg]->getResult();

  for (auto it=consumers[reg].begin(); it != consumers[reg].end(); it++) {
    DecodedInstruction consumer = it->first;
    PortIndex port = it->second;
    consumer->readRegistersCallback(port, result);
  }

  consumers[reg].clear();
}

void ForwardingNetwork::resultWritten(RegisterIndex reg) {
  producers[reg].reset();
}

void ForwardingNetwork::removeProducer(RegisterIndex reg) {
  loki_assert(producers[reg]);

  // TODO: would prefer to wait for register writing to start (not finish), but
  // that isn't exposed at the moment. Then the register file can do the work.
  // There might also be issues with multiple producers for the same register if
  // they stay here too long.
  if (producers[reg]->completedPhase(InstructionInterface::INST_REG_WRITE))
    resultProduced(reg);
    // default trigger: new producer
  else
    next_trigger(producers[reg]->finishedPhaseEvent());
}

void ForwardingNetwork::removeConsumer(RegisterIndex reg) {
  loki_assert(producers[reg]);

  if (producers[reg]->completedPhase(InstructionInterface::INST_COMPUTE))
    resultProduced(reg);
    // default trigger: new producer
  else
    next_trigger(producers[reg]->finishedPhaseEvent());
}



} /* namespace Compute */
