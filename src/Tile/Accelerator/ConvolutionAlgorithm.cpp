/*
 * ConvolutionAlgorithm.cpp
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#include "ConvolutionAlgorithm.h"
#include "../../Utility/Assert.h"

ConvolutionAlgorithm::ConvolutionAlgorithm(sc_module_name name) :
    LokiComponent(name) {

  prepareForNewInput();

}

bool ConvolutionAlgorithm::executing() const {
  return inProgress;
}

void ConvolutionAlgorithm::start(const conv_parameters_t parameters) {
  assert(!executing());

  this->parameters = parameters;
  inProgress = true;
}

sc_event ConvolutionAlgorithm::finishedComputation() const {
  return finishedComputationEvent;
}

void ConvolutionAlgorithm::sendInputCommand(const dma_command_t command) {
  loki_assert(!oInputCommand.valid());
  oInputCommand.write(command);
}

void ConvolutionAlgorithm::sendWeightsCommand(const dma_command_t command) {
  loki_assert(!oWeightsCommand.valid());
  oWeightsCommand.write(command);
}

void ConvolutionAlgorithm::sendOutputCommand(const dma_command_t command) {
  loki_assert(!oOutputCommand.valid());
  oOutputCommand.write(command);
}

void ConvolutionAlgorithm::notifyExecutionFinished() {
  finishedComputationEvent.notify(sc_core::SC_ZERO_TIME);
  prepareForNewInput();
}

void ConvolutionAlgorithm::prepareForNewInput() {
  inProgress = false;
  stepCount = 0;

  // Set all loop counters to zero.
  memset(&counters, 0, sizeof(conv_shape_t));
}
