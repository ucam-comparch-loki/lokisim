/*
 * ControlUnit.h
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_CONTROLUNIT_H_
#define SRC_TILE_ACCELERATOR_CONTROLUNIT_H_

#include "../../LokiComponent.h"
#include "ParameterReceiver.h"

class ControlUnit: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput iClock;

  // TODO connection from cores


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ControlUnit);

  ControlUnit(sc_module_name name);


//============================================================================//
// Methods
//============================================================================//

private:

  // Pass parameters to algorithm when all have been received.
  void startExecution();

  // Tell `algorithm` to `step` whenever possible. Each step will issue at least
  // one command to one of the DMA units. A step is therefore not allowed if any
  // DMA command queue is full.
  void executionStep();

  // Determine whether it is safe to prepare for the next execution step.
  // Reasons why it may not be safe include flow control signals blocking
  // progress.
  bool canStartNewStep() const;

  // Check that the parameters are consistent. This is purely for debug -
  // simulation will end with an assertion failure if anything is wrong.
  void parameterSanityCheck(const conv_parameters_t parameters);

  // Use the received parameters to update the DMA units' memory configuration.
  void updateMemoryMapping(const conv_parameters_t parameters);

  Accelerator* parent() const;


//============================================================================//
// Local state
//============================================================================//

private:

  // Component to collect parameters even while another convolution is in
  // progress.
  ParameterReceiver     receiver;

  // TODO: who is responsible for deleting this?
  ConvolutionAlgorithm* algorithm;

};

#endif /* SRC_TILE_ACCELERATOR_CONTROLUNIT_H_ */