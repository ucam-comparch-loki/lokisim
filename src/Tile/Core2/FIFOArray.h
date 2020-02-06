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

using sc_core::sc_event;

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

  void selectChannelWithData(DecodedInstruction inst, uint bitmask);

  bool hasData(RegisterIndex fifo) const;
  RegisterIndex getSelectedChannel() const;

  // Event triggered whenever any data is written to any FIFO.
  const sc_event& anyDataArrivedEvent() const;

  // Read which bypasses all normal processes and completes immediately.
  const int32_t& debugRead(RegisterIndex fifo);

  // Write which bypasses all normal processes and completes immediately.
  void debugWrite(RegisterIndex fifo, NetworkData value);

protected:

  void processRequests();

private:

  // State has changed: check whether data has arrived for one of the selected
  // channels.
  void checkBitmask();

//============================================================================//
// Local state
//============================================================================//

private:

  // A buffer for each channel-end.
  LokiVector<NetworkFIFO<Word>> fifos;

  // Allows round-robin selection of channels when executing selch.
  LoopCounter       currentChannel;

  // Bitmask representing channels available to selch. Bit 0 maps to fifos[0].
  uint              bitmask;
  DecodedInstruction selchInst;

  sc_event anyDataArrived;

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
  void write(DecodedInstruction inst, NetworkData value);

  // Event triggered when any data has been sent onto the network. For our
  // purposes, this means the data has entered the output FIFO, as this allows
  // the instruction which generated the data to continue.
  // TODO: need to ignore fetch requests? Have two ports, with priority/
  // arbitration, and only return event for one of the ports?
  const sc_event& anyDataSentEvent() const;

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

  sc_event anyDataSent;

};

} /* namespace Compute */

#endif /* SRC_TILE_CORE_FIFOARRAY_H_ */
