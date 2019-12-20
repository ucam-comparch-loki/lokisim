/*
 * FIFOArray.h
 *
 * All data inputs to the core from the network. Instruction inputs are handled
 * elsewhere.
 *
 *  Created on: Aug 19, 2019
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_FIFOARRAY_H_
#define SRC_TILE_CORE_FIFOARRAY_H_

#include "StorageBase.h"
#include "../../Network/FIFOs/NetworkFIFO.h"

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

  bool hasData(RegisterIndex fifo) const;
  void selectChannelWithData(uint bitmask);
  RegisterIndex getSelectedChannel() const;

  // Read which bypasses all normal processes and completes immediately.
  const int32_t& debugRead(RegisterIndex fifo);

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex fifo, NetworkData value);

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

  // For debug.
  int32_t local;
};



// At the core's outputs, we only ever use complete flits.
class OutputFIFOs: public StorageBase<NetworkData> {

//============================================================================//
// Ports
//============================================================================//

public:

  // Data values sent over the network.
  typedef sc_port<network_source_ifc<Word>> OutPort;
  OutPort oMemory, oMulticast;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  OutputFIFOs(sc_module_name name, const fifo_parameters_t params);

//============================================================================//
// Methods
//============================================================================//

public:

  // No need to specify a FIFO - we will use the network address to choose.
  void write(NetworkData value);

  // Read which bypasses all normal processes and completes immediately.
  const NetworkData& debugRead(RegisterIndex fifo);

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex fifo, NetworkData value);

protected:

  void processRequests();

//============================================================================//
// Local state
//============================================================================//

private:

  // A buffer for each network.
  LokiVector<NetworkFIFO<Word>> fifos;

  // For debug.
  NetworkData local;

};

} /* namespace Compute */

#endif /* SRC_TILE_CORE_FIFOARRAY_H_ */
