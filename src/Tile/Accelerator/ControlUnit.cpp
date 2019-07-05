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
#include "ConvolutionAlgorithm.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation.h"
#include "Accelerator.h"

ControlUnit::ControlUnit(sc_module_name name, const accelerator_parameters_t& params) :
    LokiComponent(name),
    iParameter("iParameter"),
    oCores("oCores"),
    receiver("params"),
    algorithm("algorithm", params),
    coreNotification("coreNotification", 1),
    notificationAddress(ChannelID(parent().id, 0)) {

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
  sensitive << parent().finishedComputationEvent();
  dont_initialize();

}

void ControlUnit::startExecution() {
  loki_assert(receiver.hasAllParameters());

  if (algorithm.executing())
    next_trigger(algorithm.finishedComputation());
  else {
    const conv_parameters_t parameters = receiver.getParameters();

    parameterSanityCheck(parameters);
    updateMemoryMapping(parameters);
    notificationAddress.write(parameters.notificationAddress);

    LOKI_LOG << this->name() << " starting computation" << endl;

    algorithm.start(parameters);

    Instrumentation::idle(parent().id, false);
  }
}

void ControlUnit::executionStep() {
  loki_assert(algorithm.executing());

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
  loki_assert(!algorithm.executing());

  LOKI_LOG << this->name() << " finished computation" << endl;

  loki_assert(coreNotification.canWrite());

  // Message doesn't need to contain any useful information.
  Flit<Word> flit(0, notificationAddress.getDestination());
  coreNotification.write(flit);

  Instrumentation::idle(parent().id, true);
}

void ControlUnit::parameterSanityCheck(const conv_parameters_t params) {
  loki_assert(params.shape.groups != 0);
  loki_assert_with_message(params.shape.inChannels % params.shape.groups == 0,
      "Channels: %d, groups: %d", params.shape.inChannels, params.shape.groups);
  loki_assert_with_message(params.shape.outChannels % params.shape.groups == 0,
      "Channels: %d, groups: %d", params.shape.outChannels, params.shape.groups);
//  loki_assert(params.shape.imageHeight >= params.shape.filterHeight);
//  loki_assert(params.shape.imageWidth >= params.shape.filterWidth);
}

void ControlUnit::updateMemoryMapping(const conv_parameters_t params) {
  parent().in1.replaceMemoryMapping(params.input.memoryConfigEncoded);
  parent().in2.replaceMemoryMapping(params.filters.memoryConfigEncoded);
  parent().out.replaceMemoryMapping(params.output.memoryConfigEncoded);
}

Accelerator& ControlUnit::parent() const {
  return *(static_cast<Accelerator*>(this->get_parent_object()));
}
