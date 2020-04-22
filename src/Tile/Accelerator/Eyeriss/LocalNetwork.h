/*
 * LocalNetwork.h
 *
 * Direct links between neighbouring PEs, used to communicate partial sums.
 * Partial sums may only be sent from a PE to the PE directly north of it, and
 * only if the layer configuration allows it.
 *
 *  Created on: 22 Apr 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_LOCALNETWORK_H_
#define SRC_TILE_ACCELERATOR_EYERISS_LOCALNETWORK_H_

#include "../../../LokiComponent.h"

namespace Eyeriss {

class LocalNetwork: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

	// TODO
	// Technically, the bottom row of PEs doesn't need an input, and the top row
	// doesn't need an output, but it's simpler if all PEs can be identical.
	// Input: one per PE
	// Output: one per PE

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

	LocalNetwork(sc_module_name name, size2d_t size);
};

} /* namespace Eyeriss */

#endif /* SRC_TILE_ACCELERATOR_EYERISS_LOCALNETWORK_H_ */
