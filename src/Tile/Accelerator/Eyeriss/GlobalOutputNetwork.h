/*
 * GlobalOutputNetwork.h
 *
 * Network responsible for gathering (partial) sums from the PEs and storing
 * them in the global buffer (L1 cache).
 *
 *  Created on: 22 Apr 2020
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_EYERISS_GLOBALOUTPUTNETWORK_H_
#define SRC_TILE_ACCELERATOR_EYERISS_GLOBALOUTPUTNETWORK_H_

#include "../../../LokiComponent.h"

namespace Eyeriss {

template<typename T>
class GlobalOutputNetwork: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

	// TODO
	// Input = one per PE?
	// Output = single port to DMA/global buffer?

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

	GlobalOutputNetwork(sc_module_name name, size2d_t size);
};

} /* namespace Eyeriss */

#endif /* SRC_TILE_ACCELERATOR_EYERISS_GLOBALOUTPUTNETWORK_H_ */
