/*
 * GlobalInputNetwork.cpp
 *
 *  Created on: 21 Apr 2020
 *      Author: db434
 */

#include "GlobalInputNetwork.h"

namespace Eyeriss {

template<typename T>
GlobalInputNetwork<T>::GlobalInputNetwork(sc_module_name name, size2d_t size) :
		LokiComponent(name),
		rowControl("row_control", size.height),
		columnControl("col_control", size.width),
		rowBuses("row_bus", size.height),
		globalBus("global_bus") {
	// TODO Auto-generated constructor stub

}

} /* namespace Eyeriss */
