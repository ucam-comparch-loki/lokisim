/*
 * InputFIFOs.h
 *
 * All data inputs to the core from the network. Instruction inputs are handled
 * elsewhere.
 *
 *  Created on: Aug 19, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_INPUTFIFOS_H_
#define SRC_TILE_CORE_INPUTFIFOS_H_

#include "../../Network/FIFOs/NetworkFIFO.h"
#include "StorageBase.h"

namespace Compute {

// We write flits to the FIFOs, but only need to read the payloads.
class InputFIFOs: public StorageBase<NetworkData, int32_t, NetworkData> {

//============================================================================//
// Ports
//============================================================================//

public:

  // Data values received over the network.
  typedef sc_port<network_sink_ifc<Word>> InPort;
  LokiVector<InPort> iData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  InputFIFOs(sc_module_name name, size_t numFIFOs,
             const fifo_parameters_t params);

//============================================================================//
// Methods
//============================================================================//

public:

  // Read which bypasses all normal processes and completes immediately.
  const int32_t& debugRead(RegisterIndex reg);

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex reg, NetworkData value);

protected:

  void processRequests();

//============================================================================//
// Local state
//============================================================================//

private:

  // A buffer for each channel-end.
  LokiVector<NetworkFIFO<Word>> fifos;

  // Allows round-robin selection of channels when executing selch.
  LoopCounter       currentChannel;
};

} /* namespace Compute */

#endif /* SRC_TILE_CORE_INPUTFIFOS_H_ */
