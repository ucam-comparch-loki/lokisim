/*
 * GlobalInputNetwork.h
 *
 * Network responsible for delivering operands to PEs. A single operand may
 * reach multiple PEs.
 *
 * A single GlobalInputNetwork is responsible for delivering a single operand
 * type, e.g. activations, weights, partial sums.
 *
 *  Created on: 21 Apr 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_GLOBALINPUTNETWORK_H_
#define SRC_TILE_ACCELERATOR_EYERISS_GLOBALINPUTNETWORK_H_

#include "../../../LokiComponent.h"
#include "../../../Utility/LokiVector.h"
#include "Interconnect.h"
#include "MulticastController.h"

namespace Eyeriss {

template<typename T>
class GlobalInputNetwork: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

	// TODO
	// Input: single? Type <tag, <tag, datat>>?
	// Output: one per PE

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

	GlobalInputNetwork(sc_module_name name, size2d_t size);

//============================================================================//
// Local state
//============================================================================//

private:

	// MulticastControllers filter out data which is useful to a subset of PEs.
	LokiVector<MulticastControllerRow<T>> rowControl; // Gate at each row's input
	LokiVector<MulticastControllerColumn<T>> columnControl; // Gate at each PE's input

	LokiVector<MulticastBus<T>> rowBuses;
	MulticastBus<pair<uint, T>> globalBus;
};

} /* namespace Eyeriss */

#endif /* SRC_TILE_ACCELERATOR_EYERISS_GLOBALINPUTNETWORK_H_ */
