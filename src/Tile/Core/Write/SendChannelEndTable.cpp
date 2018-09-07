/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "WriteStage.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../ChannelMapTable.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation/Latency.h"
#include "../../../Utility/Instrumentation/Network.h"
#include "../../../Utility/Instrumentation/Stalls.h"
#include "../Core.h"

void SendChannelEndTable::write(const NetworkData data) {

  if (data.channelID().multicast) {
    loki_assert(!bufferLocal.full());
    LOKI_LOG << this->name() << " writing " << data << " to buffer (local)\n";
    bufferLocal.write(data);
  }
  else if (core().isMemory(data.channelID().component)) {
    loki_assert(!bufferMemory.full());
    LOKI_LOG << this->name() << " writing " << data << " to buffer (memory)\n";
    Instrumentation::Latency::coreBufferedMemoryRequest(id, data);
    bufferMemory.write(data);
  }
  else {
    loki_assert(!bufferGlobal.full());
    LOKI_LOG << this->name() << " writing " << data << " to buffer (global)\n";
    bufferGlobal.write(data);

    // Check whether we're sending to a valid address.
    TileID tile = data.channelID().component.tile;
    if (!core().isComputeTile(tile)) {
      LOKI_WARN << "Preparing to send data outside bounds of simulated chip." << endl;
      LOKI_WARN << "  Source: " << id << ", destination: " << data.channelID() << endl;
//      LOKI_WARN << "  Simulating up to tile (" << TOTAL_TILE_COLUMNS-1 << "," << TOTAL_TILE_ROWS-1 << ")" << endl;
      LOKI_WARN << "  Consider increasing the COMPUTE_TILE_ROWS or COMPUTE_TILE_COLUMNS parameters." << endl;
    }
  }

  bufferFillChanged.notify();
}

bool SendChannelEndTable::full() const {
  // TODO: we would prefer to allow data to keep arriving, as long as it isn't
  // going to be put in a full buffer.
  return bufferLocal.full() || bufferMemory.full() || bufferGlobal.full();
}

const sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChanged;
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
      else if (bufferLocal.full())
        next_trigger(bufferLocal.readEvent());
      else if (bufferMemory.full())
        next_trigger(bufferMemory.readEvent());
      else if (bufferGlobal.full())
        next_trigger(bufferGlobal.readEvent());
      else {
        loki_assert(!bufferLocal.full());
        loki_assert(!bufferMemory.full());
        loki_assert(!bufferGlobal.full());

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
      else {
        loki_assert(!bufferLocal.full());
        loki_assert(!bufferMemory.full());
        loki_assert(!bufferGlobal.full());

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
  if (bufferLocal.empty()) {
    next_trigger(bufferLocal.writeEvent());
  }
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    const NetworkData data = bufferLocal.read();

    LOKI_LOG << this->name() << " sending (local) " << data << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, data.channelID().component);
    Instrumentation::Network::recordBandwidth(oDataLocal.name());

    oDataLocal.write(data);
    bufferFillChanged.notify();

    next_trigger(oDataLocal.ack_event());
  }
}

void SendChannelEndTable::sendLoopMemory() {

  switch (sendStateMemory) {
    case SS_IDLE: {

      // Remove the request for network resources if the previous data sent was
      // the end of a data packet.
      const NetworkData& data = oDataMemory.read();
      if (core().isMemory(data.channelID().component) && data.getMetadata().endOfPacket)
        requestArbitration(data.channelID(), false);

      if (bufferMemory.empty()) {
        // When will this event be triggered? Will waiting 0.6 cycles always work?
        // Can we ensure that the data always arrives at the start of the cycle?
        next_trigger(bufferMemory.writeEvent());
      }
      else {
        sendStateMemory = SS_DATA_READY;

        // Wait until slightly after the negative clock edge to request arbitration
        // because the memory updates its flow control signals on the negedge.
        next_trigger(0.6, sc_core::SC_NS);
      }

      break;
    }

    case SS_DATA_READY: {
      loki_assert(!bufferMemory.empty());

      // Request arbitration.
      requestArbitration(bufferMemory.peek().channelID(), true);
      next_trigger(clock.posedge_event());
      sendStateMemory = SS_ARBITRATING;

      break;
    }

    case SS_ARBITRATING: {
      // If the network has granted our request to send data, send it.
      // Otherwise, wait another cycle.
      if (requestGranted(bufferMemory.peek().channelID())) {
        sendStateMemory = SS_CAN_SEND;
        next_trigger(sc_core::SC_ZERO_TIME);
      }
      else {
        next_trigger(clock.posedge_event());
      }
      break;
    }

    case SS_CAN_SEND: {
      loki_assert(!bufferMemory.empty());

      const NetworkData data = bufferMemory.read();
      bufferFillChanged.notify();

      LOKI_LOG << this->name() << " sending (memory) " << data << endl;
      if (ENERGY_TRACE)
        Instrumentation::Network::traffic(id, data.channelID().component);
      Instrumentation::Network::recordBandwidth(oDataMemory.name());
      Instrumentation::Latency::coreSentMemoryRequest(id, data);

      oDataMemory.write(data);

      // Return to IDLE state and see if there is more data to send.
      sendStateMemory = SS_IDLE;
      next_trigger(oDataMemory.ack_event());

      break;
    }

  }

}

void SendChannelEndTable::sendLoopGlobal() {
  // This loop can be much simpler than the local one, above, because each
  // core has a single dedicated link, and so no arbitration is needed.

  // Wait until we have data, and only send on a positive clock edge.
  if (bufferGlobal.empty()) {
    next_trigger(bufferGlobal.writeEvent());
  }
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    const NetworkData data = bufferGlobal.read();

    LOKI_LOG << this->name() << " sending (global) " << data << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, data.channelID().component);
    Instrumentation::Network::recordBandwidth(oDataGlobal.name());

    oDataGlobal.write(data);
    bufferFillChanged.notify();

    next_trigger(oDataGlobal.ack_event());
  }
}

void SendChannelEndTable::requestArbitration(ChannelID destination, bool request) {
  parent().requestArbitration(destination, request);
}

bool SendChannelEndTable::requestGranted(ChannelID destination) const {
  return parent().requestGranted(destination);
}

void SendChannelEndTable::receivedCredit() {
  loki_assert(iCredit.valid());
  receiveCreditInternal(iCredit.read());
  iCredit.ack();
}

void SendChannelEndTable::receiveCreditInternal(const NetworkCredit& credit) {
  ChannelIndex targetCounter = credit.channelID().channel;

  LOKI_LOG << this->name() << " received credit at "
      << ChannelID(id, targetCounter) << " " << credit.messageID() << endl;

  channelMapTable->addCredit(targetCounter, credit.payload().toUInt());
}

WriteStage& SendChannelEndTable::parent() const {
  return static_cast<WriteStage&>(*(this->get_parent_object()));
}

Core& SendChannelEndTable::core() const {
  return parent().core();
}

void SendChannelEndTable::reportStalls(ostream& os) {
  if (!bufferLocal.empty()) {
    os << this->name() << " unable to send " << bufferLocal.peek() << endl;
      os << "  Waiting for arbitration request to be granted." << endl;
  }

  if (!bufferMemory.empty()) {
    os << this->name() << " unable to send " << bufferMemory.peek() << endl;
    os << "  Waiting for arbitration request to be granted." << endl;
  }

  if (!bufferGlobal.empty()) {
    os << this->name() << " unable to send " << bufferGlobal.peek() << endl;

//    if (!channelMapTable->canSend(bufferGlobal.peek().channelMapEntry()))
//      os << "  Need credits." << endl;
//    else
      os << "  Waiting for arbitration request to be granted." << endl;
  }

  // TODO: woche
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name,
                                         const ComponentID& ID,
                                         const fifo_parameters_t& fifoParams,
                                         ChannelMapTable* cmt) :
    LokiComponent(name, ID),
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

  SC_METHOD(receiveLoop);
  SC_METHOD(sendLoopLocal);
  SC_METHOD(sendLoopMemory);
  SC_METHOD(sendLoopGlobal);

  SC_METHOD(receivedCredit);
  sensitive << iCredit;
  dont_initialize();

}
