/*
 * ControlUnit.cpp
 *
 * Unit responsible for stepping through the convolution algorithm and
 * determining which data to send to which functional units at which time.
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#include "ControlUnit.h"
#include "../../Utility/Assert.h"
#include "Accelerator.h"
#include "Algorithm.h"

ControlUnit::ControlUnit(sc_module_name name, const accelerator_parameters_t& params) :
    LokiComponent(name),
    clock("clock"),
    iParameter("iParameter"),
    oCores("oCores"),
    receiver("params"),
    algorithm("algorithm", params),
    coreNotification("coreNotification", 1, 1),
    notificationAddress(ChannelID(parent().id, 0)) {

  startTime = -1;

  coreNotification.clock(clock);
  receiver.clock(clock);

  iParameter(receiver.iParameter);
  oCores(coreNotification);

  algorithm.oInputCommand(oDMA1Command);
  algorithm.oWeightsCommand(oDMA2Command);
  algorithm.oOutputCommand(oDMA3Command);

  SC_METHOD(startExecution);
  sensitive << receiver.allParametersArrived();
  dont_initialize();

  SC_METHOD(executionStep);
  sensitive << algorithm.startedComputation();
  dont_initialize();

  SC_METHOD(notifyFinished);
  sensitive << algorithm.finishedComputation();
  sensitive << parent().finishedComputationEvent();
  dont_initialize();

}

void ControlUnit::startExecution() {
  loki_assert(receiver.hasAllParameters());

  if (algorithm.executing())
    next_trigger(algorithm.finishedComputation());
  else {
    const lat_parameters_t parameters = receiver.getParameters();

    updateMemoryMapping(parameters);
    notificationAddress.write(parameters.notification_address);

    LOKI_LOG(1) << this->name() << " starting computation" << endl;
    startTime = Instrumentation::currentCycle();

    algorithm.start(parameters);

    Instrumentation::idle(parent().id, false);
  }
}

void ControlUnit::executionStep() {
  // We can get to this point and not have an algorithm to execute if there was
  // no computation to do, and so it completed immediately.
  if (!algorithm.executing())
    return;

  if (canStartNewStep())
    algorithm.step();

  // Perhaps use something more specific if the algorithm is blocked?
  if (algorithm.executing())
    next_trigger(clock.posedge_event());
  // else default trigger is algorithm.startedComputation()
}

bool ControlUnit::canStartNewStep() const {
  return algorithm.executing() &&
         parent().in1.canAcceptCommand() &&
         parent().in2.canAcceptCommand() &&
         parent().out.canAcceptCommand();
}

void ControlUnit::notifyFinished() {
  // This may not be valid if multiple computations get queued up.
  if (algorithm.executing())
    next_trigger(algorithm.finishedComputation());
  else if (!parent().isIdle())
    next_trigger(parent().finishedComputationEvent());
  else {
    LOKI_LOG(1) << this->name() << " finished computation" << endl;
    if (ACCELERATOR_TRACE)
      cout << parent().name() << " took " << Instrumentation::currentCycle() - startTime << " cycles" << endl;

    loki_assert(coreNotification.canWrite());

    // Message doesn't need to contain any useful information.
    Flit<Word> flit(0, notificationAddress.getDestination());
    coreNotification.write(flit);

    Instrumentation::idle(parent().id, true);
  }
}

void ControlUnit::updateMemoryMapping(const lat_parameters_t params) {
  parent().in1.replaceMemoryMapping(params.in1.memory_config);
  parent().in2.replaceMemoryMapping(params.in2.memory_config);
  parent().out.replaceMemoryMapping(params.out.memory_config);
}

Accelerator& ControlUnit::parent() const {
  return *(static_cast<Accelerator*>(this->get_parent_object()));
}
