/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "WriteStage.h"
#include "../Core.h"
#include "../ChannelMapTable.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation/Latency.h"
#include "../../../Utility/Instrumentation/Network.h"

void SendChannelEndTable::write(const NetworkData data) {

  if (data.channelID().multicast) {
    loki_assert(bufferMulticast.canWrite());
    bufferMulticast.write(data);
  }
  else {
    // Messages for both memory and cores on other tiles share the same buffer
    // and network.

    loki_assert(bufferData.canWrite());
    bufferData.write(data);

    if (core().isMemory(data.channelID().component))
      Instrumentation::Latency::coreBufferedMemoryRequest(id(), data);
    else {
      // Check whether we're sending to a valid address.
      TileID tile = data.channelID().component.tile;
      if (!core().isComputeTile(tile)) {
        LOKI_WARN << "Preparing to send data outside bounds of simulated chip." << endl;
        LOKI_WARN << "  Source: " << id() << ", destination: " << data.channelID() << endl;
//      LOKI_WARN << "  Simulating up to tile (" << TOTAL_TILE_COLUMNS-1 << "," << TOTAL_TILE_ROWS-1 << ")" << endl;
        LOKI_WARN << "  Consider increasing the COMPUTE_TILE_ROWS or COMPUTE_TILE_COLUMNS parameters." << endl;
      }
    }
  }
}

bool SendChannelEndTable::full() const {
  // TODO: we would prefer to allow data to keep arriving, as long as it isn't
  // going to be put in a full buffer.
  return !bufferMulticast.canWrite() || !bufferData.canWrite();
}

const sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChangedEvent;
}

void SendChannelEndTable::receiveLoop() {
  // All data is read on the negative clock edge. This gives enough time for
  // tag checks and memory bank computation to have taken place, and leaves
  // enough time to get to the arbiters and back.

  switch (receiveState) {
    case RS_READY: {
      // Wait for data to arrive
      if (!iFetch.valid() && !iData.valid())
        next_trigger(iFetch.default_event() | iData.default_event());
      // Wait for right point in clock cycle
//      else if (!clock.negedge())
//        next_trigger(clock.negedge_event());
      else if (!bufferMulticast.canWrite())
        next_trigger(bufferMulticast.canWriteEvent());
      else if (!bufferData.canWrite())
        next_trigger(bufferData.canWriteEvent());
      else {
        loki_assert(bufferMulticast.canWrite());
        loki_assert(bufferData.canWrite());

        // Choose appropriate input port - fetch has priority.
        NetworkData flit;
        if (iFetch.valid()) {
          flit = iFetch.read();
          iFetch.ack();

          LOKI_LOG(3) << this->name() << " received from fetch: " << flit << endl;
        }
        else {
          flit = iData.read();
          iData.ack();

          LOKI_LOG(3) << this->name() << " received from execute: " << flit << endl;
        }

        write(flit);

        // If the flit is part of a larger packet, wait for the whole packet to
        // come through.
        if (!flit.getMetadata().endOfPacket)
          receiveState = RS_PACKET;

        next_trigger(clock.posedge_event());
      }

      break;
    }

    case RS_PACKET: {
      // Wait for data to arrive - we can only receive packets on the data
      // input.
      if (!iData.valid())
        next_trigger(iData.default_event());
      // Wait for right point in clock cycle
//      else if (!clock.negedge())
//        next_trigger(clock.negedge_event());
      else if (!bufferMulticast.canWrite())
        next_trigger(bufferMulticast.canWriteEvent());
      else if (!bufferData.canWrite())
        next_trigger(bufferData.canWriteEvent());
      else {
        loki_assert(bufferMulticast.canWrite());
        loki_assert(bufferData.canWrite());

        NetworkData flit = iData.read();
        iData.ack();

        LOKI_LOG(3) << this->name() << " received from execute: " << flit << endl;

        write(flit);

        // Ready to start a new packet if this one has now finished.
        if (flit.getMetadata().endOfPacket)
          receiveState = RS_READY;

        next_trigger(clock.posedge_event());
      }

      break;
    }
  }
}

void SendChannelEndTable::sentMulticastData() {
  const NetworkData& flit = bufferMulticast.lastDataRead();
  if (ENERGY_TRACE)
    Instrumentation::Network::traffic(id(), flit.channelID().component);
}

void SendChannelEndTable::sentPointToPointData() {
  const NetworkData& flit = bufferData.lastDataRead();
  if (ENERGY_TRACE)
    Instrumentation::Network::traffic(id(), flit.channelID().component);

  if (core().isMemory(flit.channelID().component))
    Instrumentation::Latency::coreSentMemoryRequest(id(), flit);
}

void SendChannelEndTable::bufferFillChanged() {
  bufferFillChangedEvent.notify(sc_core::SC_ZERO_TIME);
}

ComponentID SendChannelEndTable::id() const {
  return parent().id();
}

WriteStage& SendChannelEndTable::parent() const {
  return static_cast<WriteStage&>(*(this->get_parent_object()));
}

Core& SendChannelEndTable::core() const {
  return parent().core();
}

void SendChannelEndTable::reportStalls(ostream& os) {
  if (bufferMulticast.canRead())
    os << this->name() << " unable to send " << bufferMulticast.peek() << endl;

  if (bufferData.canRead())
    os << this->name() << " unable to send " << bufferData.peek() << endl;

  // TODO: woche
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name,
                                         const fifo_parameters_t& fifoParams,
                                         uint numCores,
                                         uint numMemories) :
    LokiComponent(name),
    BlockingInterface(),
    clock("clock"),
    iFetch("iFetch"),
    iData("iData"),
    oMulticast("oDataMulticast"),
    oData("oDataMemory"),
    bufferMulticast("bufferLocal", fifoParams.size),
    bufferData("bufferMemory", fifoParams.size) {

  receiveState = RS_READY;

  oMulticast(bufferMulticast);
  oData(bufferData);

  SC_METHOD(receiveLoop);

  SC_METHOD(sentMulticastData);
  sensitive << bufferMulticast.dataConsumedEvent();
  dont_initialize();

  SC_METHOD(sentPointToPointData);
  sensitive << bufferData.dataConsumedEvent();
  dont_initialize();

  SC_METHOD(bufferFillChanged);
  sensitive << bufferMulticast.canWriteEvent() << bufferMulticast.writeEvent()
    << bufferData.canWriteEvent() << bufferData.writeEvent();
  dont_initialize();

}
