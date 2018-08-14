/*
 * ConvolutionAlgorithm.h
 *
 * Base class for all convolution algorithms. A ConvolutionAlgorithm is
 * responsible for stepping through the algorithm in PE-array-sized chunks,
 * issuing commands to the various DMA units.
 *
 * This class should be extended to implement any particular loop order.
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_CONVOLUTIONALGORITHM_H_
#define SRC_TILE_ACCELERATOR_CONVOLUTIONALGORITHM_H_

#include "../../LokiComponent.h"

typedef sc_in<dma_command_t>  CommandInput;
typedef sc_out<dma_command_t> CommandOutput;
typedef sc_signal<dma_command_t> CommandSignal;

class ConvolutionAlgorithm: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  CommandOutput oInputCommand;
  CommandOutput oWeightsCommand;
  CommandOutput oOutputCommand;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // Information we might need:
  //  * A way to access the DMA units
  //  * The size of the PE array
  ConvolutionAlgorithm(sc_module_name name);


//============================================================================//
// Methods
//============================================================================//

public:

  // Is this algorithm currently in progress?
  bool executing() const;

  // Prepare to execute a new convolution.
  void start(const conv_parameters_t parameters);

  // "Execute" one iteration of the algorithm and send relevant commands to the
  // DMA units.
  // TODO: should this be abstract, or is it possible to generalise the
  // different loop orders?
  virtual void step() = 0;

  // Event triggered when all execution is finished.
  sc_event finishedComputation() const;

protected:

  // Methods to hide the use of ports from the algorithm. These are the
  // preferred way of sending commands to the DMA units.
  void sendInputCommand(const dma_command_t command);
  void sendWeightsCommand(const dma_command_t command);
  void sendOutputCommand(const dma_command_t command);

private:

  // Perform any tidying when the final iteration has completed.
  void notifyExecutionFinished();

  // Clear any temporary data ready for new parameters.
  void prepareForNewInput();


//============================================================================//
// Local state
//============================================================================//

protected:

  // The received computation parameters. Used as loop bounds. Remain constant
  // while execution is in progress.
  conv_parameters_t parameters;

  // A local set of parameters used to hold the current loop indices.
  conv_shape_t counters;

  // Step counter. One step = one computation by each PE.
  count_t stepCount;

private:

  bool inProgress;
  sc_event finishedComputationEvent;

};

#endif /* SRC_TILE_ACCELERATOR_CONVOLUTIONALGORITHM_H_ */
