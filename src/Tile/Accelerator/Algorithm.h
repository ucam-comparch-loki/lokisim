/*
 * Algorithm.h
 *
 * An Algorithm is responsible for stepping through the required computation in
 * PE-array-sized chunks, issuing commands to the various DMA units.
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ALGORITHM_H_
#define SRC_TILE_ACCELERATOR_ALGORITHM_H_

#include "../../LokiComponent.h"
#include "../../Memory/MemoryTypes.h"
#include "AcceleratorTypes.h"

typedef sc_in<dma_command_t>  CommandInput;
typedef sc_out<dma_command_t> CommandOutput;
typedef sc_signal<dma_command_t> CommandSignal;

// Details required to execute a single loop.
typedef struct {
  uint iterations; // Total number of iterations
  uint current;    // Current iteration
  int  in1Skip;    // Distance between elements of first data type in bytes
  int  in2Skip;    // Distance between elements of second data type in bytes
  int  outSkip;    // Distance between elements of output in bytes
} loop_t;

class Algorithm: public LokiComponent {

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
  Algorithm(sc_module_name name,
                       const accelerator_parameters_t& params);


//============================================================================//
// Methods
//============================================================================//

public:

  // Is this algorithm currently in progress?
  bool executing() const;

  // Prepare to execute a new convolution.
  void start(const lat_parameters_t parameters);

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
  vector<loop_t> getLoops(const lat_parameters_t& parameters) const;

  // Check whether the given loops are compatible with the current hardware
  // configuration. Modify the computation plan if necessary.
  void checkLoops(const vector<loop_t>& loops);

//============================================================================//
// Local state
//============================================================================//

protected:

  // The received computation parameters. Used as loop bounds. Remain constant
  // while execution is in progress.
  lat_parameters_t parameters;

  // Details for each of the loops in the loop nest.
  vector<loop_t> loopNest;

  // Step counter. One step = one computation by each PE.
  tick_t stepCount;

  // Depending on the configuration chosen, we may not be able to use all of the
  // PEs.
  size2d_t activePEs;

private:

  bool inProgress;
  sc_event startedComputationEvent;
  sc_event finishedComputationEvent;

  // Configuration of the accelerator. Contains information such as size of PE
  // array.
  const accelerator_parameters_t& config;

};

#endif /* SRC_TILE_ACCELERATOR_ALGORITHM_H_ */