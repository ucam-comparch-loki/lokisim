/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "WriteStage.h"
#include "../Core.h"
#include "../../ComputeTile.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../ChannelMapTable.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation/Latency.h"
#include "../../../Utility/Instrumentation/Network.h"
#include "../../../Utility/Instrumentation/Stalls.h"

void SendChannelEndTable::write(const NetworkData data) {

  if (data.channelID().multicast) {
    loki_assert(bufferLocal.canWrite());
    bufferLocal.write(data);
  }
  else if (core().isMemory(data.channelID().component)) {
    loki_assert(bufferMemory.canWrite());
    Instrumentation::Latency::coreBufferedMemoryRequest(id(), data);
    bufferMemory.write(data);
  }
  else {
    loki_assert(bufferGlobal.canWrite());
    bufferGlobal.write(data);

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

bool SendChannelEndTable::full() const {
  // TODO: we would prefer to allow data to keep arriving, as long as it isn't
  // going to be put in a full buffer.
  return !bufferLocal.canWrite() || !bufferMemory.canWrite() || !bufferGlobal.canWrite();
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
      else if (!bufferLocal.canWrite())
        next_trigger(bufferLocal.canWriteEvent());
      else if (!bufferMemory.canWrite())
        next_trigger(bufferMemory.canWriteEvent());
      else if (!bufferGlobal.canWrite())
        next_trigger(bufferGlobal.canWriteEvent());
      else {
        loki_assert(bufferLocal.canWrite());
        loki_assert(bufferMemory.canWrite());
        loki_assert(bufferGlobal.canWrite());

        // Choose appropriate input port - fetch has priority.
        NetworkData flit;
        if (iFetch.valid()) {
          flit = iFetch.read();
          iFetch.ack();

          LOKI_LOG << this->name() << " received from fetch: " << flit << endl;
        }
        else {
          flit = iData.read();
          iData.ack();

          LOKI_LOG << this->name() << " received from execute: " << flit << endl;
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
      else if (!bufferLocal.canWrite())
        next_trigger(bufferLocal.canWriteEvent());
      else if (!bufferMemory.canWrite())
        next_trigger(bufferMemory.canWriteEvent());
      else if (!bufferGlobal.canWrite())
        next_trigger(bufferGlobal.canWriteEvent());
      else {
        loki_assert(bufferLocal.canWrite());
        loki_assert(bufferMemory.canWrite());
        loki_assert(bufferGlobal.canWrite());

        NetworkData flit = iData.read();
        iData.ack();

        LOKI_LOG << this->name() << " received from execute: " << flit << endl;

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

void SendChannelEndTable::sendLoopLocal() {
  // There is no arbitration on the core-to-core network. Just send data
  // when the output is free.

  // Wait until we have data, and only send on a positive clock edge.
  if (!bufferLocal.dataAvailable()) {
    next_trigger(bufferLocal.dataAvailableEvent());
  }
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    const NetworkData data = bufferLocal.read();

    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id(), data.channelID().component);
    Instrumentation::Network::recordBandwidth(oDataLocal.name());

    oDataLocal.write(data);

    next_trigger(oDataLocal.ack_event());
  }
}

void SendChannelEndTable::sendLoopGlobal() {
  // This loop can be much simpler than the local one, above, because each
  // core has a single dedicated link, and so no arbitration is needed.

  // Wait until we have data, and only send on a positive clock edge.
  if (!bufferGlobal.dataAvailable()) {
    next_trigger(bufferGlobal.dataAvailableEvent());
  }
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    const NetworkData data = bufferGlobal.read();

    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id(), data.channelID().component);
    Instrumentation::Network::recordBandwidth(oDataGlobal.name());

    oDataGlobal.write(data);

    next_trigger(oDataGlobal.ack_event());
  }
}

void SendChannelEndTable::sentMemoryRequest() {
  const NetworkData& flit = bufferMemory.lastDataRead();
  if (ENERGY_TRACE)
    Instrumentation::Network::traffic(id(), flit.channelID().component);
  Instrumentation::Latency::coreSentMemoryRequest(id(), flit);
}

void SendChannelEndTable::requestCoreAccess(ChannelID destination,
                                            bool request) {
  // Do nothing - this network does not require arbitration.
}

bool SendChannelEndTable::coreRequestGranted(ChannelID destination) const {
  // No arbitration on this network.
  return true;
}

void SendChannelEndTable::requestGlobalAccess(ChannelID destination,
                                              bool request) {
  // Do nothing - this network does not require arbitration.
}

bool SendChannelEndTable::globalRequestGranted(ChannelID destination) const {
  // No arbitration on this network.
  return true;
}

void SendChannelEndTable::receivedCredit() {
  loki_assert(iCredit.valid());
  receiveCreditInternal(iCredit.read());
  iCredit.ack();
}

void SendChannelEndTable::bufferFillChanged() {
  bufferFillChangedEvent.notify(sc_core::SC_ZERO_TIME);
}

void SendChannelEndTable::receiveCreditInternal(const NetworkCredit& credit) {
  ChannelIndex targetCounter = credit.channelID().channel;

  LOKI_LOG << this->name() << " received credit at "
      << ChannelID(id(), targetCounter) << " " << credit.messageID() << endl;

  channelMapTable->addCredit(targetCounter, credit.payload().toUInt());
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
  if (bufferLocal.dataAvailable()) {
    os << this->name() << " unable to send " << bufferLocal.peek() << endl;
      os << "  Waiting for arbitration request to be granted." << endl;
  }

  if (bufferMemory.dataAvailable()) {
    os << this->name() << " unable to send " << bufferMemory.peek() << endl;
    os << "  Waiting for arbitration request to be granted." << endl;
  }

  if (bufferGlobal.dataAvailable()) {
    os << this->name() << " unable to send " << bufferGlobal.peek() << endl;

//    if (!channelMapTable->canSend(bufferGlobal.peek().channelMapEntry()))
//      os << "  Need credits." << endl;
//    else
      os << "  Waiting for arbitration request to be granted." << endl;
  }

  // TODO: woche
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name,
                                         const fifo_parameters_t& fifoParams,
                                         ChannelMapTable* cmt,
                                         uint numCores,
                                         uint numMemories) :
    LokiComponent(name),
    BlockingInterface(),
    clock("clock"),
    iFetch("iFetch"),
    iData("iData"),
    oDataLocal("oDataMulticast"),
    oDataMemory("oDataMemory"),
    oDataGlobal("oDataGlobal"),
    iCredit("iCredit"),
    bufferLocal(string(this->name())+string(".bufferLocal"), fifoParams.size),
    bufferMemory(string(this->name())+string(".bufferMemory"), fifoParams.size),
    bufferGlobal(string(this->name())+string(".bufferGlobal"), fifoParams.size),
    channelMapTable(cmt) {

  receiveState = RS_READY;
  sendStateMemory = SS_IDLE;
  sendStateMulticast = SS_IDLE;

  oDataMemory(bufferMemory);

  SC_METHOD(receiveLoop);
  SC_METHOD(sendLoopLocal);
  SC_METHOD(sendLoopGlobal);

  SC_METHOD(sentMemoryRequest);
  sensitive << bufferMemory.dataConsumedEvent();
  dont_initialize();

  SC_METHOD(receivedCredit);
  sensitive << iCredit;
  dont_initialize();

  // TODO: remove this when all buffers are properly hooked up to the networks.
  SC_METHOD(bufferFillChanged);
  sensitive << bufferLocal.canWriteEvent() << bufferLocal.dataAvailableEvent()
    << bufferMemory.canWriteEvent() << bufferMemory.dataAvailableEvent()
    << bufferGlobal.canWriteEvent() << bufferGlobal.dataAvailableEvent();
  dont_initialize();

}
