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
  if (DEBUG)
    cout << this->name() << " writing " << data << " to buffer\n";

  if (data.channelID().multicast || (data.channelID().component.tile == id.tile)) {
    assert(!bufferLocal.full());
    bufferLocal.write(data);
    bufferFillChanged.notify();
  }
  else {
    assert(!bufferGlobal.full());
    bufferGlobal.write(data);
  }
}

bool SendChannelEndTable::full() const {
  // TODO: we would prefer to allow data to keep arriving, as long as it isn't
  // going to be put in a full buffer.
  return bufferLocal.full() || bufferGlobal.full() || waiting;
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
        if (!flit.endOfPacket())
          receiveState = RS_PACKET;

        next_trigger(clock.posedge_event());
      }

      break;
    }

    case RS_PACKET: {
      // Port = iData
      // Wait for data to arrive
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
        if (flit.endOfPacket())
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
      if (data.endOfPacket())
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
        // fall through to SS_CAN_SEND state
      }
      else {
        next_trigger(clock.posedge_event());
        break;
      }
    }
    /* no break */

    case SS_CAN_SEND: {
      assert(!bufferLocal.empty());

      const NetworkData data = bufferLocal.read();
      bufferFillChanged.notify();

      if (DEBUG)
        cout << this->name() << " sending (local) " << data << endl;
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

  // Wait until we have data, we have the credits to send that data, and only
  // send on a positive clock edge.
  if (bufferGlobal.empty())
    next_trigger(bufferGlobal.writeEvent());
  else if (!clock.posedge())
    next_trigger(clock.posedge_event());
  else {
    const NetworkData data = bufferGlobal.read();

    if (DEBUG)
      cout << this->name() << " sending (global) " << data << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, data.channelID().component);

    oDataGlobal.write(data);

    next_trigger(oDataGlobal.ack_event());
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel, const DecodedInst& inst) {
  Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Wait until the channel's credit counter reaches its maximum value, if it
  // is using credits.
  next_trigger(channelMapTable->allCreditsEvent(channel));

  // TODO: split this method into two (at this point) and perhaps integrate
  // with the main loop.

  Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT, inst);
  waiting = false;
  bufferFillChanged.notify(); // No it didn't - use separate events?
}

void SendChannelEndTable::requestArbitration(ChannelID destination, bool request) {
  parent()->requestArbitration(destination, request);
}

bool SendChannelEndTable::requestGranted(ChannelID destination) const {
  return parent()->requestGranted(destination);
}

void SendChannelEndTable::receivedCredit() {
  assert(iCredit.valid());

  ChannelIndex targetCounter = iCredit.read().channelID().channel;

  if (DEBUG)
    cout << this->name() << " received credit at " << ChannelID(id, targetCounter) << " " << iCredit.read().messageID() << endl;

  channelMapTable->addCredit(targetCounter);
  iCredit.ack();
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
    Blocking(),
    bufferLocal(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".bufferLocal")),
    bufferGlobal(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".bufferGlobal")),
    channelMapTable(cmt) {

  receiveState = RS_READY;
  sendState = SS_IDLE;

  waiting = false;

  SC_METHOD(receiveLoop);
  SC_METHOD(sendLoopLocal);
  SC_METHOD(sendLoopGlobal);

  SC_METHOD(receivedCredit);
  sensitive << iCredit;
  dont_initialize();

}
