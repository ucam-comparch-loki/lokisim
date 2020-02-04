/*
 * NetworkAccess.h
 *
 * Mix-ins which allow access to the on-chip network.
 *
 * Inputs received from the network are mapped to registers, so require no
 * special treatment. These mix-ins therefore focus on sending data onto the
 * network.
 *
 *  Created on: 6 Aug 2019
 *      Author: db434
 */

#ifndef SRC_ISA_MIXINS_NETWORKACCESS_H_
#define SRC_ISA_MIXINS_NETWORKACCESS_H_
#include "OperandSource.h"

// Basic network access - for use by ALU operations, for example.
// Simply take the `result` and send it.
template<class T>
class NetworkSend : public T {
public:
  NetworkSend(Instruction encoded) : T(encoded) {}

  // Override the ReadCMT implementation to ensure there are enough credits
  // available before continuing.
  // Can't call through to ReadCMT::readCMTCallback because it ends the phase
  // immediately.
  void readCMTCallback(EncodedCMTEntry value) {
    this->channelMapping = value;
    checkIfReady();
  }
  void creditArrivedCallback() {
    checkIfReady();
  }

  void sendNetworkData() {
    this->startingPhase(this->INST_NETWORK_SEND);

    if (this->outChannel == NO_CHANNEL)
      this->finishedPhase(this->INST_NETWORK_SEND);
    else {
      int32_t payload = this->result;
      ChannelID destination =
          this->core->getNetworkDestination(this->channelMapping, payload);

      NetworkData flit;

      // Send to cores on this tile.
      if (destination.multicast) {
        flit = NetworkData(payload, destination, false, false, true);
      }
      // Send to a core over the global network.
      else if (ChannelMapEntry::isCore(this->channelMapping)) {
        ChannelMapEntry::GlobalChannel channel(this->channelMapping);
        flit = NetworkData(payload, destination, channel.acquired, false, true);
      }
      // Send to memory on this tile.
      else {
        assert(ChannelMapEntry::isMemory(this->channelMapping));
        flit = NetworkData(payload,
                           destination,
                           ChannelMapEntry::memoryView(this->channelMapping),
                           LOAD_W,  // Sending a plain message = load word
                           true);
      }

      this->core->sendOnNetwork(flit);
    }
  }
  void sendNetworkDataCallback() {
    this->finishedPhase(this->INST_NETWORK_SEND);
  }

  bool sendsOnNetwork() const {
    return this->outChannel != NO_CHANNEL;
  }

protected:
  // Block the instruction until there are enough credits to send the result
  // (if using a network which uses credits).
  void checkIfReady() {
    assert(this->outChannel != NO_CHANNEL);

    bool useCredits = ChannelMapEntry::globalView(this->channelMapping).isGlobal;

    if (useCredits && this->core->creditsAvailable(this->outChannel) == 0)
      this->core->waitForCredit(this->outChannel);
    else
      this->finishedPhase(this->INST_CMT_READ);
  }
};


// Send config - use the `sendconfig` instruction to precisely control which
// metadata is sent with the flit.
template<class T>
class SendConfig : public Has2Operands<T> {
public:
  SendConfig(Instruction encoded) : Has2Operands<T>(encoded) {}

  void sendNetworkData() {
    this->startingPhase(this->INST_NETWORK_SEND);

    int32_t payload = this->operand1;
    uint32_t metadata = this->operand2;
    ChannelID destination =
        this->core->getNetworkDestination(this->channelMapping, payload);

    NetworkData flit(payload, destination, metadata);
    this->core->sendOnNetwork(flit);
  }

  void sendNetworkDataCallback() {
    this->finishedPhase(this->INST_NETWORK_SEND);
  }
};


// Send data specifically to memory banks. All memory operations must provide
// some extra fields to allow this.
template<class T>
class MemorySend : public T {
public:
  MemorySend(Instruction encoded) : T(encoded) {flitsSent = 0;}

  void sendNetworkData() {
    assert(this->totalFlits > 0);
    this->startingPhase(this->INST_NETWORK_SEND);

    int32_t payload = this->getPayload(flitsSent);
    MemoryOpcode op = this->getMemoryOp(flitsSent);
    ChannelMapEntry::MemoryChannel metadata(this->channelMapping);
    bool eop = (flitsSent == (this->totalFlits - 1));

    // Compute the network destination for the first flit, assuming the payload
    // is a memory address. Use the same destination for all subsequent flits.
    if (flitsSent == 0)
      networkDestination =
          this->core->getNetworkDestination(this->channelMapping, payload);

    NetworkData flit(payload, networkDestination, metadata, op, eop);
    this->core->sendOnNetwork(flit);
  }

  void sendNetworkDataCallback() {
    flitsSent++;
    if (flitsSent == this->totalFlits)
      this->finishedPhase(this->INST_NETWORK_SEND);
  }

protected:
  uint flitsSent;
  ChannelID networkDestination;
};


#endif /* SRC_ISA_MIXINS_NETWORKACCESS_H_ */