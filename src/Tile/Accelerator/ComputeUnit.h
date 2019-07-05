/*
 * ComputeUnit.h
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_
#define SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_

#include "../../LokiComponent.h"
#include "../../Network/NetworkTypes.h"
#include "../../Utility/LokiVector.h"
#include "AcceleratorChannel.h"
#include "AcceleratorTypes.h"
#include "ComputeStage.h"

class Accelerator;

template<typename T>
class ComputeUnit: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  typedef sc_port<accelerator_consumer_ifc<T>> InPort;
  typedef sc_port<accelerator_producer_ifc<T>> OutPort;

  ClockInput clock;

  // The data being computed.
  InPort in1, in2;
  OutPort out;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ComputeUnit(sc_module_name name, const accelerator_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

protected:

  // Connect a new stage to the existing pipeline. Assumes a single input which
  // receives data from the most-recently-added stage.
  void addNewStage(ComputeStage<T>* stage, size2d_t inputSize);

  Accelerator& parent() const;
  const ComponentID& id() const;

//============================================================================//
// Local state
//============================================================================//

private:

  friend class ComputeStage<T>;

  LokiVector<ComputeStage<T>> pipeline;
  LokiVector<AcceleratorChannel<T>> channels;

};

#endif /* SRC_TILE_ACCELERATOR_COMPUTEUNIT_H_ */
