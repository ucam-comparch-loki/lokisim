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
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Instrumentation/Network.h"
#include "../../../Utility/Instrumentation/Stalls.h"

void SendChannelEndTable::write(const DecodedInst& data) {
  if (DEBUG)
    cout << this->name() << " writing " << data.result() << " to output buffer\n";

  assert(!buffer.full());

  buffer.write(data);
  bufferFillChanged.notify();
}

bool SendChannelEndTable::full() const {
  return buffer.full() || waiting;
}

const sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChanged;
}

void SendChannelEndTable::sendLoop() {

  switch (state) {
    case IDLE: {

      // Remove the request for network resources if the previous data sent was
      // the end of a data packet.
      const AddressedWord& data = output[0].read();
      if (data.endOfPacket())
        requestArbitration(data.channelID(), false);

      if (buffer.empty()) {
        // When will this event be triggered? Will waiting 0.6 cycles always work?
        // Can we ensure that the data always arrives at the start of the cycle?
        next_trigger(buffer.writeEvent());
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
      assert(!buffer.empty());

      // Before requesting network resources, we must first make sure that the
      // destination core is ready to receive more data, and that if we are
      // issuing an instruction packet fetch, there is space in the local cache.
      if (!channelMapTable->canSend(buffer.peek().channelMapEntry())) {
        next_trigger(1.0, sc_core::SC_NS);
      }
      else if ((buffer.peek().memoryOp() == MemoryRequest::IPK_READ) &&
               !parent()->readyToFetch()) {
        next_trigger(1.0, sc_core::SC_NS);
      }
      else {
        // Request arbitration.
        requestArbitration(buffer.peek().networkDestination(), true);
        next_trigger(clock.posedge_event());
        state = ARBITRATING;
      }

      break;
    }

    case ARBITRATING: {
      // If the network has granted our request to send data, send it.
      // Otherwise, wait another cycle.
      if (requestGranted(buffer.peek().networkDestination())) {
        state = CAN_SEND;
        // fall through to CAN_SEND state
      }
      else {
        next_trigger(clock.posedge_event());
        break;
      }
    }

    case CAN_SEND: {
      assert(!buffer.empty());

      const DecodedInst& inst = buffer.read();
      bufferFillChanged.notify();

      // Send data
      const AddressedWord data = inst.toAddressedWord();

      if (DEBUG)
        cout << this->name() << " sending " << data << endl;
      if (ENERGY_TRACE)
        Instrumentation::Network::traffic(id, data.channelID().getComponentID());

      output[0].write(data);
      channelMapTable->removeCredit(inst.channelMapEntry());

      // Return to IDLE state and see if there is more data to send.
      state = IDLE;
      next_trigger(output[0].ack_event());

      break;
    }

  }

}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::Stalls::stall(id, Instrumentation::Stalls::STALL_OUTPUT);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Wait until the channel's credit counter reaches its maximum value, if it
  // is using credits.
  channelMapTable->waitForAllCredits(channel);

  Instrumentation::Stalls::unstall(id);
  waiting = false;
  bufferFillChanged.notify(); // No it didn't - use separate events?
}

void SendChannelEndTable::requestArbitration(ChannelID destination, bool request) {
  parent()->requestArbitration(destination, request);
}

bool SendChannelEndTable::requestGranted(ChannelID destination) const {
  return parent()->requestGranted(destination);
}

void SendChannelEndTable::receivedCredit(unsigned int buffer) {
  assert(creditsIn[buffer].valid());

  ChannelIndex targetCounter = creditsIn[buffer].read().channelID().getChannel();

  if (DEBUG)
    cout << "Received credit at " << ChannelID(id, targetCounter) << endl;

  channelMapTable->addCredit(buffer);
}

void SendChannelEndTable::creditFromCores()    {receivedCredit(TO_CORES);}
void SendChannelEndTable::creditFromMemories() {} // Memories don't send credits
void SendChannelEndTable::creditFromOffTile()  {receivedCredit(OFF_TILE);}

WriteStage* SendChannelEndTable::parent() const {
  return static_cast<WriteStage*>(this->get_parent());
}

SendChannelEndTable::SendChannelEndTable(sc_module_name name, const ComponentID& ID, ChannelMapTable* cmt) :
    Component(name, ID),
    buffer(OUT_CHANNEL_BUFFER_SIZE, string(name)),
    channelMapTable(cmt) {

  state = IDLE;

  static const unsigned int NUM_BUFFERS = CORE_OUTPUT_PORTS;

  output.init(NUM_BUFFERS);
  creditsIn.init(NUM_BUFFERS);

  waiting = false;

  SC_METHOD(sendLoop);

  SC_METHOD(creditFromCores);  sensitive << creditsIn[0];  dont_initialize();

}
