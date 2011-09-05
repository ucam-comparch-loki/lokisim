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
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/Instrumentation/Stalls.h"

bool SendChannelEndTable::write(const DecodedInst& dec) {

  if(dec.sendsOnNetwork())
    write(dec.toAddressedWord(), dec.channelMapEntry());
  else if (dec.operation() == InstructionMap::WOCHE)
		waitUntilEmpty(dec.result());
	
	return true;
}

bool SendChannelEndTable::write(const AddressedWord& data, MapIndex output) {
  unsigned int bufferToUse = (int)channelMapTable->getNetwork(output);

  // We know it is safe to write to any buffer because we block the rest
  // of the pipeline if a buffer is full.
  assert(!buffers[bufferToUse].full());
  assert(output < CHANNEL_MAP_SIZE);

  buffers[bufferToUse].write(BufferedInfo(data, output));
  dataToSendEvent[bufferToUse].notify();
  bufferFillChanged.notify();

  if(DEBUG) cout << this->name() << " wrote " << data
                 << " to output buffer " << bufferToUse << endl;

  return data.endOfPacket();
}

bool SendChannelEndTable::full() const {
  bool full = false;
  for(unsigned int i=0; i<buffers.size(); i++) {
    if(buffers[i].full()) {
      full = true;
      break;
    }
  }

  return full || waiting;
}

const sc_event& SendChannelEndTable::stallChangedEvent() const {
  return bufferFillChanged;
}

void SendChannelEndTable::sendToCores()    {sendLoop(TO_CORES);}
void SendChannelEndTable::sendToMemories() {sendLoop(TO_MEMORIES);}
void SendChannelEndTable::sendOffTile()    {sendLoop(OFF_TILE);}

void SendChannelEndTable::sendLoop(unsigned int buffer) {
  if(ackOutput[buffer].posedge()) {
    // If we are receiving an acknowledgement, we have just sent some data.
    assert(validOutput[buffer].read());
    validOutput[buffer].write(false);

    if(buffers[buffer].empty())        // No data to send.
      next_trigger(dataToSendEvent[buffer]);
    else if(clock.posedge())  // Have data and it's time to send it.
      send(buffer);
    else                      // Have data but it's not time to send it.
      next_trigger(clock.posedge_event());
  }
  else {
    // If we are not receiving an acknowledgement, we must have data to send.
    assert(!buffers[buffer].empty());

    send(buffer);
  }
}

void SendChannelEndTable::send(unsigned int buffer) {
  assert(!buffers[buffer].empty());

  MapIndex entry = buffers[buffer].peek().second;

  // If we don't have enough credits, abandon sending the data.
  if(!channelMapTable->canSend(entry)) {
    next_trigger(clock.posedge_event());
    return;
  }

  AddressedWord data = buffers[buffer].read().first;

  if(DEBUG) cout << "Sending " << data.payload()
      << " from " << ChannelID(id, (int)entry) << " to "
      << data.channelID() << endl;

  output[buffer].write(data);
  validOutput[buffer].write(true);
  channelMapTable->removeCredit(entry);

  bufferFillChanged.notify();

  next_trigger(ackOutput[buffer].posedge_event());
}

/* Stall the pipeline until the specified channel is empty. */
void SendChannelEndTable::waitUntilEmpty(MapIndex channel) {
  Instrumentation::stalled(id, true, Stalls::OUTPUT);
  waiting = true;
  bufferFillChanged.notify(); // No it didn't - use separate events?

  // Wait until the channel's credit counter reaches its maximum value, if it
  // is using credits.
  channelMapTable->waitForAllCredits(channel);

  Instrumentation::stalled(id, false);
  waiting = false;
  bufferFillChanged.notify(); // No it didn't - use separate events?
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
    buffers(NUM_BUFFERS, OUT_CHANNEL_BUFFER_SIZE, string(name)) {

  channelMapTable = cmt;

  output      = new sc_out<AddressedWord>[NUM_BUFFERS];
  validOutput = new sc_out<bool>[NUM_BUFFERS];
  ackOutput   = new sc_in<bool>[NUM_BUFFERS];

  creditsIn   = new sc_in<AddressedWord>[NUM_BUFFERS];
  validCredit = new sc_in<bool>[NUM_BUFFERS];

  waiting = false;

  dataToSendEvent = new sc_event[NUM_BUFFERS];

  SC_METHOD(sendToCores);        sensitive << dataToSendEvent[0];   dont_initialize();
  SC_METHOD(sendToMemories);     sensitive << dataToSendEvent[1];   dont_initialize();
  SC_METHOD(sendOffTile);        sensitive << dataToSendEvent[2];   dont_initialize();

  SC_METHOD(creditFromCores);    sensitive << validCredit[0].pos(); dont_initialize();
  SC_METHOD(creditFromMemories); sensitive << validCredit[1].pos(); dont_initialize();
  SC_METHOD(creditFromOffTile);  sensitive << validCredit[2].pos(); dont_initialize();

}

SendChannelEndTable::~SendChannelEndTable() {
  delete[] output;
  delete[] validOutput;
  delete[] ackOutput;

  delete[] creditsIn;
  delete[] validCredit;

  delete[] dataToSendEvent;
}
