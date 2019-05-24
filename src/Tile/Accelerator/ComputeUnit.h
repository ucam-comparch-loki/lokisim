/*
 * ComputeUnit.h
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_
#define SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_

#include "../../LokiComponent.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Operations.h"
#include "../../Utility/LokiVector2D.h"
#include "AcceleratorTypes.h"
#include "AdderTree.h"
#include "Multiplier.h"

class Accelerator;

template<typename T>
class ComputeUnit: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput clock;

  // The data being computed.
  LokiVector2D<sc_in<T>> in1;
  LokiVector2D<sc_in<T>> in2;
  LokiVector2D<sc_out<T>> out;

  // The input from another component is ready to be consumed.
  ReadyInput  in1Ready;
  ReadyInput  in2Ready;

  // The component receiving the output is ready to consume new data.
  ReadyInput  outReady;

  // The current tick being executed. This signal also acts as flow control,
  // with any connected components only supplying data when the tick reaches a
  // predetermined value.
  // We have separate signals for inputs and outputs because the computation
  // may be pipelined, leaving the output a few ticks behind the input.
  sc_out<tick_t> inputTick, outputTick;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ComputeUnit<T>);
  ComputeUnit(sc_module_name name, const accelerator_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

private:

  void newTick();

  void triggerCompute();

  // Signal that the output for the given tick is available on the `out` ports.
  void newOutputTick();

  Accelerator& parent() const;
  const ComponentID& id() const;

//============================================================================//
// Local state
//============================================================================//

private:

  LokiVector2D<Multiplier<T>> multipliers;
  LokiVector<AdderTree<T>> adders;
  LokiVector<sc_signal<T>> signals;

  tick_t currentTick;

  sc_event outputReadyEvent;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_ */
