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
#include "../../Memory/MemoryTypes.h"
#include "AcceleratorTypes.h"
#include "Configuration.h"
#include "Loops.h"

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
  ConvolutionAlgorithm(sc_module_name name, const Configuration& config);


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
  void step();

  // Event triggered when the start() method has been called.
  const sc_event& startedComputation() const;

  // Event triggered when all execution is finished.
  const sc_event& finishedComputation() const;

protected:

  // Methods to hide the use of ports from the algorithm. These are the
  // preferred way of sending commands to the DMA units.
  void sendIn1Command(const dma_command_t command);
  void sendIn2Command(const dma_command_t command);
  void sendOutCommand(const dma_command_t command);

private:

  // Perform any tidying when the final iteration has completed.
  void notifyExecutionFinished();

  // Clear any temporary data ready for new parameters.
  void prepareForNewInput();

  // Get details for each of the loops in the computation, regardless of their
  // order. These loops are arranged in such a way that a Loop enum value can
  // be used to select a loop which iterates along a particular dimension.
  vector<loop_t> getUnorderedLoops(const conv_parameters_t parameters) const;

  // Put the loop details into the order required.
  vector<loop_t> reorderLoops(const vector<loop_t>& unordered, const LoopOrder& order) const;

//============================================================================//
// Local state
//============================================================================//

protected:

  // The received computation parameters. Used as loop bounds. Remain constant
  // while execution is in progress.
  conv_parameters_t parameters;

  // Details for each of the loops in the loop nest.
  vector<loop_t> loopNest;

  // Step counter. One step = one computation by each PE.
  tick_t stepCount;

private:

  bool inProgress;
  sc_event startedComputationEvent;
  sc_event finishedComputationEvent;

  // Configuration of the accelerator. Contains information such as loop order
  // and size of PE array.
  const Configuration& config;

};

#endif /* SRC_TILE_ACCELERATOR_CONVOLUTIONALGORITHM_H_ */
