/*
 * EyerissStage.cpp
 *
 *  Created on: 21 Apr 2020
 *      Author: db434
 */

#include "EyerissStage.h"

namespace Eyeriss {

// Latency is 0, meaning 0 additional cycles. The internal components have their
// own latency.
template<typename T>
EyerissStage<T>::EyerissStage(sc_module_name name, size2d_t size):
	ComputeStage<T>(name, 2, 0, 1),
	pes("pe", size.height, size.width), // TODO: might need to init manually
	activations("activations", size),
	weights("weights", size),
	psums("psums", size),
	control("control", size.height, size.width) {
	// TODO Auto-generated constructor stub

}

} /* namespace Eyeriss */
