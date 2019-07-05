/*
 * ComputeStage.h
 *
 * Base class for all pipeline stages in the Accelerator's ComputeUnit.
 *
 *  Created on: 4 Jul 2019
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_COMPUTESTAGE_H_
#define SRC_TILE_ACCELERATOR_COMPUTESTAGE_H_

#include "../../LokiComponent.h"
#include "../../Utility/BlockingInterface.h"
#include "../../Utility/LokiVector.h"
#include "Interface.h"

using sc_core::sc_port;

template<typename T> class ComputeUnit;

// TODO Should there be separate types for the inputs and outputs? Perhaps even
// separate types for each input?
template <typename T>
class ComputeStage: public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  typedef sc_port<accelerator_consumer_ifc<T>> InPort;
  typedef sc_port<accelerator_producer_ifc<T>> OutPort;

  // TODO clock?

  LokiVector<InPort> in;
  OutPort out;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ComputeStage);

  // The stage produces an output `latency` cycles after it receives an input,
  // and is able to accept new input every `initiationInterval` cycles.
  ComputeStage(sc_module_name name, uint numInputs, uint latency,
               uint initiationInterval);


//============================================================================//
// Methods
//============================================================================//

protected:

  // Extra initialisation once all the ports are bound.
  virtual void end_of_elaboration();

  // Convert inputs to outputs. To be overridden by each subclass.
  virtual void compute() = 0;

  // Ensure all inputs and outputs are ready, and then trigger the compute
  // method.
  virtual void mainLoop();

  // Print debug information if this component is unable to make progress.
  virtual void reportStalls(ostream& os);


  ComputeUnit<T>& parent() const;

  const ComponentID& id() const;


//============================================================================//
// Local state
//============================================================================//

private:

  // Delay between consuming inputs and producing an output.
  const sc_time latency;

  // Delay between consuming consecutive inputs on the same port.
  const sc_time initiationInterval;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTESTAGE_H_ */
