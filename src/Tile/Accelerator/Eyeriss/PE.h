/*
 * Eyeriss::PE.h
 *
 * Implementation of Eyeriss's PE based on:
 *   https://ieeexplore.ieee.org/document/7738524 fig 12
 *
 * It performs a 1D convolution between a row of weights and a row of a filter,
 * producing a stream of partial sums as output.
 *
 *  Created on: Feb 11, 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_PE_H_
#define SRC_TILE_ACCELERATOR_EYERISS_PE_H_

#include "../../../LokiComponent.h"
#include "../../../Network/FIFOs/FIFO.h"
#include "Configuration.h"
#include "Interconnect.h"

namespace Eyeriss {

class PE: public LokiComponent {

  // The original Eyeriss was 16 bit - we fix it at 32 bits for now.
  typedef int32_t input_t;
  typedef int32_t weight_t;
  typedef int32_t output_t;

  // TODO: In the paper, some interconnects carry multiple values at once.
  // Activations in: 1x
  // Weights in:     4x
  // PSums in:       4x
  // PSums out:      4x
  typedef sc_port<interconnect_read_ifc<input_t>>   ActivationInput;
  typedef sc_port<interconnect_read_ifc<weight_t>>  WeightInput;
  typedef sc_port<interconnect_read_ifc<output_t>>  PSumInput;
  typedef sc_port<interconnect_write_ifc<output_t>> PSumOutput;

  typedef sc_port<configuration_pe_ifc>             ControlInput;

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput      clock;

  ControlInput    control;

  ActivationInput activationInput;
  WeightInput     weightInput;
  PSumInput       psumInput;

  PSumOutput      psumOutput;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(PE);
  PE(sc_module_name name);

//============================================================================//
// Methods
//============================================================================//

private:

  void reconfigure();
  void newActivationArrived();
  void newWeightArrived();
  void newPSumArrived();
  void sendPSums();

  void activationFlowControl();
  void weightFlowControl();
  void psumFlowControl();

//============================================================================//
// Local state
//============================================================================//

private:

  FIFO<input_t> activationInputFIFO;
  FIFO<weight_t> weightInputFIFO;
  FIFO<output_t> psumInputFIFO;
  FIFO<output_t> psumOutputFIFO;

  // Local data storage.
  vector<input_t> inputSpad;
  vector<weight_t> weightSpad;
  vector<output_t> outputSpad;

  // Configuration details

  // Details of the computation to be performed - determines how the local
  // scratchpads are to be accessed.
  uint numFilters;
  uint numChannels;

  // Multiplying two fixed-point inputs will produce a wider output. Specify how
  // far the result must be right-shifted during normalisation.
  uint outputShift;
};

} // end Eyeriss namespace

#endif /* SRC_TILE_ACCELERATOR_EYERISS_PE_H_ */
