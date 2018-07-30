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

ControlUnit::ControlUnit(sc_module_name name) :
    LokiComponent(name),
    receiver("params") {

  // TODO connect up receiver's ports
  // TODO construct/receive an algorithm from somewhere
  algorithm = NULL;

  SC_METHOD(startExecution);
  sensitive << receiver.allParametersArrived();
  dont_initialize();

  SC_METHOD(executionStep);
  // sensitive << iClock?; algorithm.readyToStart()? TODO
  dont_initialize();

}

void ControlUnit::startExecution() {
  loki_assert(receiver.hasAllParameters());

  if (algorithm->executing())
    next_trigger(algorithm->finishedComputation());
  else {
    const conv_parameters_t parameters = receiver.getParameters();

    parameterSanityCheck(parameters);
    updateMemoryMapping(parameters);

    algorithm->start(parameters);
  }
}

void ControlUnit::executionStep() {
  loki_assert(algorithm->executing());

  if (canStartNewStep())
    algorithm->step(); // TODO: next_trigger?
  else
    next_trigger(iClock.posedge_event()); // TODO: something more specific?
}

bool ControlUnit::canStartNewStep() const {
  return algorithm->executing() &&
         parent()->in1.canAcceptCommand() &&
         parent()->in2.canAcceptCommand() &&
         parent()->out.canAcceptCommand();
}

void ControlUnit::parameterSanityCheck(const conv_parameters_t params) {
  loki_assert(params.shape.inChannels % params.shape.groups == 0);
  loki_assert(params.shape.outChannels % params.shape.groups == 0);
  loki_assert(params.shape.imageHeight > params.shape.filterHeight);
  loki_assert(params.shape.imageWidth > params.shape.filterWidth);
}

void ControlUnit::updateMemoryMapping(const conv_parameters_t params) {
  parent()->in1.replaceMemoryMapping(params.input.memoryConfigEncoded);
  parent()->in2.replaceMemoryMapping(params.filters.memoryConfigEncoded);
  parent()->out.replaceMemoryMapping(params.output.memoryConfigEncoded);
}

Accelerator* ControlUnit::parent() const {
  return static_cast<Accelerator*>(this->get_parent_object());
}
