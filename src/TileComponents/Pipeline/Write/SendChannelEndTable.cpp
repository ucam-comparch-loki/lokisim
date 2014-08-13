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

void SendChannelEndTable::write(const DecodedInst& data) {
  if (DEBUG)
    cout << this->name() << " writing " << data.toAddressedWord() << " to buffer\n";

  if (data.networkDestination().getTile() == id.getTile()) {
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

void SendChannelEndTable::sendLoopLocal() {

  switch (state) {
    case IDLE: {

      // Remove the request for network resources if the previous data sent was
      // the end of a data packet.
      const AddressedWord& data = oDataLocal.read();
      if (data.endOfPacket())
        requestArbitration(data.channelID(), false);

      if (bufferLocal.empty()) {
        // When will this event be triggered? Will waiting 0.6 cycles always work?
        // Can we ensure that the data always arrives at the start of the cycle?
        next_trigger(bufferLocal.writeEvent());
      }
      else {
        state = DATA_READY;

        // Wait until slightly after the negative clock edge to request arbitration
        // because the memory updates its flow control signals on the negedge.
        next_trigger(0.6, sc_core::SC_NS);
      }

      break;
    }

    case DATA_READY: {
      assert(!bufferLocal.empty());

      // Before requesting network resources, we must first make sure that if
      // we are issuing an instruction packet fetch, there is space in the
      // local cache.

      if ((bufferLocal.peek().memoryOp() == MemoryRequest::IPK_READ) &&
               !parent()->readyToFetch()) {
        next_trigger(1.0, sc_core::SC_NS);
      }
      else {
        // Request arbitration.
        requestArbitration(bufferLocal.peek().networkDestination(), true);
        next_trigger(clock.posedge_event());
        state = ARBITRATING;
      }

      break;
    }

    case ARBITRATING: {
      // If the network has granted our request to send data, send it.
      // Otherwise, wait another cycle.
      if (requestGranted(bufferLocal.peek().networkDestination())) {
        state = CAN_SEND;
        // fall through to CAN_SEND state
      }
      else {
        next_trigger(clock.posedge_event());
        break;
      }
    }
    /* no break */

    case CAN_SEND: {
      assert(!bufferLocal.empty());

      const DecodedInst& inst = bufferLocal.read();
      bufferFillChanged.notify();

      // Send data
      const AddressedWord data = inst.toAddressedWord();

      if (DEBUG)
        cout << this->name() << " sending " << data << endl;
      if (ENERGY_TRACE)
        Instrumentation::Network::traffic(id, data.channelID().getComponentID());

      oDataLocal.write(data);

      // The local network doesn't use credits.
//      channelMapTable->removeCredit(inst.channelMapEntry());

      // Return to IDLE state and see if there is more data to send.
      state = IDLE;
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
  if (bufferGlobal.empty()) {
    next_trigger(bufferGlobal.writeEvent());
  }
  else if (!channelMapTable->canSend(bufferGlobal.peek().channelMapEntry())) {
    next_trigger(iCredit.default_event());
  }
  else if (!clock.posedge()) {
    next_trigger(clock.posedge_event());
  }
  else {
    const DecodedInst& inst = bufferGlobal.read();
    const AddressedWord data = inst.toAddressedWord();

    if (DEBUG)
      cout << this->name() << " sending " << data << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, data.channelID().getComponentID());

    oDataGlobal.write(data);
    channelMapTable->removeCredit(inst.channelMapEntry());

    next_trigger(oDataGlobal.ack_event());
  }
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Wait until the channel's credit counter reaches its maximum value, if it
  // is using credits.
  next_trigger(channelMapTable->haveAllCredits(channel));

  // TODO: split this method into two (at this point) and perhaps integrate
  // with the main loop.

  Instrumentation::Stalls::unstall(id, Instrumentation::Stalls::STALL_OUTPUT);
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

  ChannelIndex targetCounter = iCredit.read().channelID().getChannel();

  if (DEBUG)
    cout << this->name() << " received credit at " << ChannelID(id, targetCounter) << " " << iCredit.read().messageID() << endl;

  channelMapTable->addCredit(targetCounter);
  iCredit.ack();
}

WriteStage* SendChannelEndTable::parent() const {
  return static_cast<WriteStage*>(this->get_parent());
}

void SendChannelEndTable::reportStalls(ostream& os) {
  if (!bufferLocal.empty()) {
    os << this->name() << " unable to send " << bufferLocal.peek().toAddressedWord() << endl;

    if (!channelMapTable->canSend(bufferLocal.peek().channelMapEntry()))
      os << "  Need credits." << endl;
    else
      os << "  Waiting for arbitration request to be granted." << endl;
  }

  if (!bufferGlobal.empty()) {
    os << this->name() << " unable to send " << bufferGlobal.peek().toAddressedWord() << endl;

    if (!channelMapTable->canSend(bufferGlobal.peek().channelMapEntry()))
      os << "  Need credits." << endl;
    else
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

  state = IDLE;

  waiting = false;

  SC_METHOD(sendLoopLocal);
  SC_METHOD(sendLoopGlobal);

  SC_METHOD(receivedCredit);
  sensitive << iCredit;
  dont_initialize();

}
