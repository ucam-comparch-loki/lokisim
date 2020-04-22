/*
 * EyerissStage.h
 *
 * An Eyeriss accelerator unit packaged up to implement the ComputeStage
 * interface. This allows it to be used as part of a compute pipeline, for
 * example, followed by a pooling layer or activation function.
 *
 *  Created on: 21 Apr 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_EYERISSSTAGE_H_
#define SRC_TILE_ACCELERATOR_EYERISS_EYERISSSTAGE_H_

#include "../../../Utility/LokiVector2D.h"
#include "../ComputeStage.h"
#include "Configuration.h"
#include "GlobalInputNetwork.h"
#include "GlobalOutputNetwork.h"
#include "PE.h"

namespace Eyeriss {

template<typename T>
class EyerissStage: public ComputeStage<T> {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

	EyerissStage(sc_module_name name, size2d_t size);

//============================================================================//
// Local state
//============================================================================//

private:

	// All PEs. Access using pes[row][column].
	LokiVector2D<PE> pes;

	// Deliver operands to/from PEs.
	GlobalInputNetwork<T> activations;
	GlobalInputNetwork<T> weights;
	GlobalInputNetwork<T> psums;
	GlobalOutputNetwork<T> outputs;

	ControlFabric control;

};

} /* namespace Eyeriss */

#endif /* SRC_TILE_ACCELERATOR_EYERISS_EYERISSSTAGE_H_ */
