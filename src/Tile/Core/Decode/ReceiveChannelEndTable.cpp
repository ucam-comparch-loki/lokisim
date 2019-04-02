/*
 * ReceiveChannelEndTable.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "ReceiveChannelEndTable.h"
#include "DecodeStage.h"
#include "../Core.h"
#include "../RegisterFile.h"
#include "../../../Datatype/Word.h"
#include "../../../Exceptions/BlockedException.h"
#include "../../../Utility/Assert.h"
#include "../../../Utility/Instrumentation/Latency.h"
#include "../../../Utility/Instrumentation/Stalls.h"

typedef RegisterFile Registers;

int32_t ReceiveChannelEndTable::read(ChannelIndex channelEnd) {
  loki_assert_with_message(channelEnd < buffers.size(), "Channel %d", channelEnd);
  loki_assert(buffers[channelEnd].canRead());

  int32_t result = buffers[channelEnd].read().payload().toInt();
  return result;
}

int32_t ReceiveChannelEndTable::readInternal(ChannelIndex channelEnd) const {
  loki_assert_with_message(channelEnd < buffers.size(), "Channel %d", channelEnd);

  if (!buffers[channelEnd].canRead())
    return 0;
  else
    return buffers[channelEnd].peek().payload().toInt();
}

void ReceiveChannelEndTable::writeInternal(ChannelIndex channel, int32_t data) {
  loki_assert_with_message(channel < buffers.size(), "Channel %d", channel);
  loki_assert(buffers[channel].canWrite());

  // TODO: currently dealing with integers rather than flits. Switch over to be
  // more consistent.
  Flit<Word> flit(data, Core::RCETInput(id(), channel));
  buffers[channel].write(flit);

  newData.notify(sc_core::SC_ZERO_TIME);
}

/* Return whether or not the specified channel contains data. */
bool ReceiveChannelEndTable::testChannelEnd(ChannelIndex channelEnd) const {
  loki_assert_with_message(channelEnd < buffers.size(), "Channel %d", channelEnd);
  return buffers[channelEnd].canRead();
}

ChannelIndex ReceiveChannelEndTable::selectChannelEnd(unsigned int bitmask, const DecodedInst& inst) {
  // Wait for data to arrive on one of the channels we're interested in.
  waitForData(bitmask, inst);

  int startPoint = currentChannel.value();

  // Check all of the channels in a round-robin style, using a LoopCounter.
  // Return the register-mapping of the first channel which has data.
  for (int i = ++currentChannel; i != startPoint; ++currentChannel) {
    i = currentChannel.value();
    if (((bitmask >> i) & 1) && buffers[i].canRead()) {
        // Adjust address so it can be accessed like a register
        return parent().core().regs.fromChannelID(i);
    }
  }

  // We should always find something in one of the buffers, because we wait
  // until at least one of them has data.
  loki_assert(false);
  return -1;
}

void ReceiveChannelEndTable::waitForData(unsigned int bitmask, const DecodedInst& inst) {
  // Wait for data to arrive on one of the channels we're interested in.
  while (true) {
    for (ChannelIndex i=0; i<buffers.size(); i++) {
      if (((bitmask >> i) & 1) && buffers[i].canRead())
        return;
    }

    // Since multiple channels are involved, assume data can come from anywhere.
    Instrumentation::Stalls::stall(id(), Instrumentation::Stalls::STALL_MEMORY_DATA, inst);
    Instrumentation::Stalls::stall(id(), Instrumentation::Stalls::STALL_CORE_DATA, inst);
    wait(newData);
    Instrumentation::Stalls::unstall(id(), Instrumentation::Stalls::STALL_MEMORY_DATA, inst);
    Instrumentation::Stalls::unstall(id(), Instrumentation::Stalls::STALL_CORE_DATA, inst);
  }
}

const sc_event& ReceiveChannelEndTable::receivedDataEvent(ChannelIndex buffer) const {
  return buffers[buffer].writeEvent();
}

ComponentID ReceiveChannelEndTable::id() const {
  return parent().id();
}

DecodeStage& ReceiveChannelEndTable::parent() const {
  return static_cast<DecodeStage&>(*(this->get_parent_object()));
}

void ReceiveChannelEndTable::reportStalls(ostream& os) {
  for (uint i=0; i<buffers.size(); i++) {
    if (!buffers[i].canWrite())
      os << buffers[i].name() << " is full." << endl;
  }
}

void ReceiveChannelEndTable::networkDataArrived(ChannelIndex buffer) {
  Instrumentation::Latency::coreReceivedResult(id(), buffers[buffer].lastDataWritten());
  newData.notify(sc_core::SC_ZERO_TIME);
}

ReceiveChannelEndTable::ReceiveChannelEndTable(const sc_module_name& name,
                                               size_t numChannels,
                                               const fifo_parameters_t& fifoParams) :
    LokiComponent(name),
    BlockingInterface(),
    clock("clock"),
    iData("iData", numChannels),
    currentChannel(numChannels) {

  for (uint i=0; i<numChannels; i++) {
    std::stringstream bufName;
    ChannelID channel = Core::RCETInput(id(), i);
    bufName << "buffer_" << (uint)channel.channel;
    NetworkFIFO<Word>* fifo = new NetworkFIFO<Word>(bufName.str().c_str(), fifoParams.size);
    buffers.push_back(fifo);

    iData[i](buffers[i]);

    SPAWN_METHOD(buffers[i].writeEvent(), ReceiveChannelEndTable::networkDataArrived, i, false);
  }

}
