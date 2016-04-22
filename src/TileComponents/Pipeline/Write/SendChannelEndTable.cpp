/*
 * SendChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "SendChannelEndTable.h"
#include "WriteStage.h"
#include "../ChannelMapTable.h"
#include "../../TileComponent.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Instrumentation/Network.h"
#include "../../../Utility/Instrumentation/Stalls.h"

void SendChannelEndTable::write(const NetworkData data) {

  if (data.channelID().multicast || (data.channelID().component.tile == id.tile)) {
    assert(!bufferLocal.full());
    LOKI_LOG << this->name() << " writing " << data << " to buffer (local)\n";
    bufferLocal.write(data);
  }
  else {
    assert(!bufferGlobal.full());
    LOKI_LOG << this->name() << " writing " << data << " to buffer (global)\n";
    bufferGlobal.write(data);

    // Check whether we're sending to a valid address.
    TileID tile = data.channelID().component.tile;
    if ((tile.x<1) || (tile.y<1) || (tile.x>COMPUTE_TILE_COLUMNS) || (tile.y>COMPUTE_TILE_ROWS)) {
      LOKI_WARN << "Preparing to send data outside bounds of simulated chip." << endl;
      LOKI_WARN << "  Source: " << id << ", destination: " << data.channelID() << endl;
      LOKI_WARN << "  Simulating up to tile (" << COMPUTE_TILE_COLUMNS << "," << COMPUTE_TILE_ROWS << ")" << endl;
      LOKI_WARN << "  Consider increasing the COMPUTE_TILE_ROWS or COMPUTE_TILE_COLUMNS parameters." << endl;
    }
  }

  bufferFillChanged.notify();
}

bool SendChannelEndTable::full() const {
  // TODO: we would prefer to allow data to keep arriving, as long as it isn't
  // going to be put in a full buffer.
  return bufferLocal.full() || bufferGlobal.full();
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
      else if (bufferGlobal.full())
        next_trigger(bufferGlobal.readEvent());
      else {
        assert(!bufferLocal.full());
        assert(!bufferGlobal.full());

        // Choose appropriate input port - fetch has priority.
        NetworkData flit;
        if (iFetch.valid()) {
          flit = iFetch.read();
          iFetch.ack();
        }
        else {
          flit = iData.read();
          iData.ack();
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
        assert(!bufferLocal.full());
        NetworkData flit = iData.read();
        iData.ack();
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

  switch (sendState) {
    case SS_IDLE: {

      // Remove the request for network resources if the previous data sent was
      // the end of a data packet.
      const NetworkData& data = oDataLocal.read();
      if (data.getMetadata().endOfPacket)
        requestArbitration(data.channelID(), false);

      if (bufferLocal.empty()) {
        // When will this event be triggered? Will waiting 0.6 cycles always work?
        // Can we ensure that the data always arrives at the start of the cycle?
        next_trigger(bufferLocal.writeEvent());
      }
      else {
        sendState = SS_DATA_READY;

        // Wait until slightly after the negative clock edge to request arbitration
        // because the memory updates its flow control signals on the negedge.
        next_trigger(0.6, sc_core::SC_NS);
      }

      break;
    }

    case SS_DATA_READY: {
      assert(!bufferLocal.empty());

      // Request arbitration.
      requestArbitration(bufferLocal.peek().channelID(), true);
      next_trigger(clock.posedge_event());
      sendState = SS_ARBITRATING;

      break;
    }

    case SS_ARBITRATING: {
      // If the network has granted our request to send data, send it.
      // Otherwise, wait another cycle.
      if (requestGranted(bufferLocal.peek().channelID())) {
        sendState = SS_CAN_SEND;
        next_trigger(sc_core::SC_ZERO_TIME);
      }
      else {
        next_trigger(clock.posedge_event());
      }
      break;
    }

    case SS_CAN_SEND: {
      assert(!bufferLocal.empty());

      const NetworkData data = bufferLocal.read();
      bufferFillChanged.notify();

      LOKI_LOG << this->name() << " sending (local) " << data << endl;
      if (ENERGY_TRACE)
        Instrumentation::Network::traffic(id, data.channelID().component);

      oDataLocal.write(data);

      // Return to IDLE state and see if there is more data to send.
      sendState = SS_IDLE;
      next_trigger(oDataLocal.ack_event());

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

    oDataGlobal.write(data);
    bufferFillChanged.notify();

    next_trigger(oDataGlobal.ack_event());
  }
}

void SendChannelEndTable::requestArbitration(ChannelID destination, bool request) {
  parent()->requestArbitration(destination, request);
}

bool SendChannelEndTable::requestGranted(ChannelID destination) const {
  return parent()->requestGranted(destination);
}

void SendChannelEndTable::receivedCredit() {
  assert(iCredit.valid());
  receiveCreditInternal(iCredit.read());
  iCredit.ack();
}

void SendChannelEndTable::receiveCreditInternal(const NetworkCredit& credit) {
  ChannelIndex targetCounter = credit.channelID().channel;

  LOKI_LOG << this->name() << " received credit at "
      << ChannelID(id, targetCounter) << " " << credit.messageID() << endl;

  channelMapTable->addCredit(targetCounter, credit.payload().toUInt());
}

WriteStage* SendChannelEndTable::parent() const {
  return static_cast<WriteStage*>(this->get_parent_object());
}

void SendChannelEndTable::reportStalls(ostream& os) {
  if (!bufferLocal.empty()) {
    os << this->name() << " unable to send " << bufferLocal.peek() << endl;

//    if (!channelMapTable->canSend(bufferLocal.peek().channelMapEntry()))
//      os << "  Need credits." << endl;
//    else
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

SendChannelEndTable::SendChannelEndTable(sc_module_name name, const ComponentID& ID, ChannelMapTable* cmt) :
    Component(name, ID),
    BlockingInterface(),
    bufferLocal(string(this->name())+string(".bufferLocal"), CORE_BUFFER_SIZE),
    bufferGlobal(string(this->name())+string(".bufferGlobal"), CORE_BUFFER_SIZE),
    channelMapTable(cmt) {

  receiveState = RS_READY;
  sendState = SS_IDLE;

  SC_METHOD(receiveLoop);
  SC_METHOD(sendLoopLocal);
  SC_METHOD(sendLoopGlobal);

  SC_METHOD(receivedCredit);
  sensitive << iCredit;
  dont_initialize();

}
