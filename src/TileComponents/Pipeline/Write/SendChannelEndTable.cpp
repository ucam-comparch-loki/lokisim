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
#include "../../../Utility/Instrumentation/Stalls.h"

void SendChannelEndTable::write(const DecodedInst& data) {
  if(DEBUG) cout << this->name() << " writing " << data.result() << " to output buffer\n";

  assert(!buffer.full());

  buffer.write(data);
  dataToSendEvent.notify();
  bufferFillChanged.notify();
}

bool SendChannelEndTable::full() const {
  return buffer.full() || waiting;
}

const sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChanged;
}

void SendChannelEndTable::sendLoop() {

  // Data becomes invalid in the second half of the clock cycle. Hack?
  if(!clock.read() && validOutput[0].read())
    validOutput[0].write(false);

  switch(state) {
    case IDLE: {

      if(buffer.empty()) {
        // When will this event be triggered? Will waiting 0.6 cycles always work?
        // Can we ensure that the data always arrives at the start of the cycle?
        next_trigger(dataToSendEvent);
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
      if(!channelMapTable->canSend(buffer.peek().channelMapEntry())) {
        next_trigger(1.0, sc_core::SC_NS);
      }
      else if((buffer.peek().memoryOp() == MemoryRequest::IPK_READ) &&
              !parent()->readyToFetch()) {
        next_trigger(1.0, sc_core::SC_NS);
      }
      else {
        // Request arbitration and wait for response.
        requestArbitration(buffer.peek().networkDestination(), true);
        state = ARBITRATING;
      }

      break;
    }

    case ARBITRATING: {
      // Wait for clock edge before sending data
      next_trigger(clock.posedge_event());
      state = CAN_SEND;

      break;
    }

    case CAN_SEND: {
      assert(!buffer.empty());

      // Wait until an arbitrarily small time after the clock edge to allow the
      // network's select signals to settle down.
      if(clock.posedge()) {
        next_trigger(0.05, sc_core::SC_NS);
        break;
      }

      const DecodedInst& inst = buffer.read();
      bufferFillChanged.notify();

      // Send data
      AddressedWord data = inst.toAddressedWord();

      if(DEBUG) cout << this->name() << " sending " << data << endl;

      output[0].write(data);
      validOutput[0].write(true);
      channelMapTable->removeCredit(inst.channelMapEntry());

      // If this was the final flit in the packet, remove the request for
      // network resources.
      if(data.endOfPacket())
        requestArbitration(data.channelID(), false);

      // Return to IDLE state immediately to see if there is more data to send.
      state = IDLE;
      next_trigger(sc_core::SC_ZERO_TIME);

      break;
    }

  }

}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::stalled(id, true, Stalls::STALL_OUTPUT);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Wait until the channel's credit counter reaches its maximum value, if it
  // is using credits.
  channelMapTable->waitForAllCredits(channel);

  Instrumentation::stalled(id, false);
  waiting = false;
  bufferFillChanged.notify(); // No it didn't - use separate events?
}

void SendChannelEndTable::requestArbitration(ChannelID destination, bool request) {
  if(request) {
    // Call execute() again when the request is granted.
    next_trigger(parent()->requestArbitration(destination, request));
  }
  else {
    // We are not sending a request which will be granted, so don't use
    // next_trigger this time.
    parent()->requestArbitration(destination, request);
  }
}

void SendChannelEndTable::receivedCredit(unsigned int buffer) {
  assert(validCredit[buffer].read());

  ChannelIndex targetCounter = creditsIn[buffer].read().channelID().getChannel();

  if(DEBUG) cout << "Received credit at " << ChannelID(id, targetCounter) << endl;

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
    buffer(OUT_CHANNEL_BUFFER_SIZE, string(name)) {

  channelMapTable = cmt;
  state = IDLE;

  static const unsigned int NUM_BUFFERS = CORE_OUTPUT_PORTS;

  output      = new sc_out<AddressedWord>[NUM_BUFFERS];
  validOutput = new sc_out<bool>[NUM_BUFFERS];

  creditsIn   = new sc_in<AddressedWord>[NUM_BUFFERS];
  validCredit = new sc_in<bool>[NUM_BUFFERS];

  waiting = false;

  SC_METHOD(sendLoop);

  SC_METHOD(creditFromCores);    sensitive << validCredit[0].pos(); dont_initialize();
//  SC_METHOD(creditFromMemories); sensitive << validCredit[1].pos(); dont_initialize();
//  SC_METHOD(creditFromOffTile);  sensitive << validCredit[2].pos(); dont_initialize();

}

SendChannelEndTable::~SendChannelEndTable() {
  delete[] output;
  delete[] validOutput;

  delete[] creditsIn;
  delete[] validCredit;
}
