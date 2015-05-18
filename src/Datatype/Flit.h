/*
* Flit.h
*
* Information to be sent on the network. Includes a payload and a network
* address.
*
* Other out-of-band data is also included for use on some networks.
*
*  Created on: 5 Jan 2010
*      Author: db434
*/

#ifndef FLIT_H_
#define FLIT_H_

#include "../Typedefs.h"
#include "Identifier.h"
#include "MemoryRequest.h"
#include "Word.h"
#include "../TileComponents/ChannelMapEntry.h"

// The basic flit just has a single end-of-packet bit. All specialisations must
// have this bit in the same position, but may use some of the "padding" bits
// to convey extra information.
struct FlitMetadata {
  unsigned int padding : 31;
  unsigned int endOfPacket : 1;

  uint32_t flatten() {
    return (padding << 1) | (endOfPacket << 0);
  }

  FlitMetadata() : FlitMetadata(0) {}

  FlitMetadata(uint32_t flattened) {
    padding = (flattened >> 1) & 0x7FFFFFFF;
    endOfPacket = (flattened >> 0) & 0x1;
  }
};

// Data sent between cores on different tiles.
struct CoreMetadata {
  unsigned int padding : 29;
  unsigned int acquired : 1;    // Has a connection already been set up?
  unsigned int allocate : 1;    // Is this message creating/removing a connection?
  unsigned int endOfPacket : 1;

  uint32_t flatten() const {
    return (padding << 3) | (acquired << 2) | (allocate << 1) | (endOfPacket << 0);
  }

  CoreMetadata() : CoreMetadata(0) {}

  CoreMetadata(uint32_t flattened) {
    padding = (flattened >> 3) & 0x1FFFFFFF;
    acquired = (flattened >> 2) & 0x1;
    allocate = (flattened >> 1) & 0x1;
    endOfPacket = (flattened >> 0) & 0x1;
  }
};

struct MemoryMetadata {
  unsigned int padding : 13;
  unsigned int returnTileX : 3;   // L2 mode only: tile of requesting L1 bank.
  unsigned int returnTileY : 3;   // L2 mode only: tile of requesting L1 bank.
  unsigned int returnChannel : 3; // L1 mode: channel of core. L2 mode: bank of L1 tile.
  unsigned int checkL2Tags : 1;   // Treat L2 as cache (1) or scratchpad (0).
  unsigned int checkL1Tags : 1;   // Treat L1 as cache (1) or scratchpad (0).
  unsigned int skipL2 : 1;        // Bypass L2 and go straight to main memory.
  unsigned int skipL1 : 1;        // Bypass L1 and go straight to L2.
  MemoryRequest::MemoryOperation opcode : 5; // Operation to perform at memory.
  unsigned int endOfPacket : 1;

  uint32_t flatten() {
    return (padding << 19) | (returnTileX << 16) | (returnTileY << 13) |
      (returnChannel << 10) | (checkL2Tags << 9) | (checkL1Tags << 8) |
      (skipL2 << 7) | (skipL1 << 6) | (opcode << 1) | (endOfPacket << 0);
  }

  MemoryMetadata() : MemoryMetadata(0) {}

  MemoryMetadata(uint32_t flattened) {
    padding = (flattened >> 19) & 0x1FFF;
    returnTileX = (flattened >> 16) & 0x7;
    returnTileY = (flattened >> 13) & 0x7;
    returnChannel = (flattened >> 10) & 0x7;
    checkL2Tags = (flattened >> 9) & 0x1;
    checkL1Tags = (flattened >> 8) & 0x1;
    skipL2 = (flattened >> 7) & 0x1;
    skipL1 = (flattened >> 6) & 0x1;
    opcode = MemoryRequest::MemoryOperation((flattened >> 1) & 0x1F);
    endOfPacket = (flattened >> 0) & 0x1;
  }
};

template <typename T>
class Flit {
private:
  // Give each network message a unique ID. Don't really care about overflow
  // because messages don't last very long.
  static uint     messageCount;
  uint            messageID_;  // Not const because we need to copy it.

  // The data being transmitted.
  T               payload_;

  // Any extra information being sent with the payload, such as whether this
  // flit is the last in the packet, or the type of memory operation to perform.
  uint32_t        metadata_;

  // The location the data is being transmitted to.
  ChannelID       channelID_;

public:

  inline uint           messageID()   const  {return messageID_;}
  inline T              payload()     const  {return payload_;}
  inline ChannelID      channelID()   const  {return channelID_;}

  // Different views of the metadata.
  inline uint32_t       getRawMetadata() const    {return metadata_;}
  inline FlitMetadata   getMetadata() const       {return FlitMetadata(metadata_);}
  inline CoreMetadata   getCoreMetadata() const   {return CoreMetadata(metadata_);}
  inline MemoryMetadata getMemoryMetadata() const {return MemoryMetadata(metadata_);}

  inline void setMetadata(const uint32_t info)         {metadata_ = info;}
  inline void setChannelID(const ChannelID id)         {channelID_ = id;}

  friend void sc_trace(sc_core::sc_trace_file*& tf, const Flit<T>& w, const std::string& txt) {
    sc_trace(tf, w.payload_, txt + ".payload");
    //sc_core::sc_trace(tf, w.channelID_, txt + ".channelID");
  }

  /* Necessary functions/operators to pass this datatype down a channel */

  inline bool operator== (const Flit<T>& other) const {
    return (this->messageID_    == other.messageID_) // is this line sufficient?
        && (this->payload_      == other.payload_)
        && (this->metadata_     == other.metadata_)
        && (this->channelID_    == other.channelID_);
  }

  inline Flit<T>& operator= (const Flit<T>& other) {
    messageID_    = other.messageID_;
    payload_      = other.payload_;
    metadata_     = other.metadata_;
    channelID_    = other.channelID_;

    return *this;
  }

  friend std::ostream& operator<< (std::ostream& os, Flit<T> const& v) {
    os << "[" << v.payload() << " -> " << v.channelID().getString() << "] (id:" << v.messageID() << ")";
    return os;
  }

  Flit<T>() :
      messageID_(messageCount++),
      payload_(static_cast<T>(0)),
      metadata_(0),
      channelID_(ChannelID(0, 0, 0, 0)) {
  }

  Flit<T>(T payload, ChannelID destination) :
      messageID_(messageCount++),
      payload_(payload),
      metadata_(0),
      channelID_(destination) {

  }

  Flit<T>(T payload, ChannelID destination, bool endOfPacket) :
      messageID_(messageCount++),
      payload_(payload),
      channelID_(destination) {

    FlitMetadata info;
    info.endOfPacket = endOfPacket;
    setMetadata(info.flatten());

  }
  
  Flit<T>(T payload, ChannelID destination, uint32_t metadata) :
      messageID_(messageCount++),
      payload_(payload),
      metadata_(metadata),
      channelID_(destination) {

  }

  // Constructor for core-to-core messages.
  Flit<T>(T payload, ChannelID destination, bool acquired, bool allocate, bool eop) :
      messageID_(messageCount++),
      payload_(payload),
      channelID_(destination) {
    CoreMetadata data;
    data.acquired = acquired;
    data.allocate = allocate;
    data.endOfPacket = eop;
    setMetadata(data.flatten());
  }

  // Constructor for messages between cores and memory.
  // TODO: get information from channel map table.
  Flit<T>(T payload, ChannelID destination, ChannelIndex returnChannel,
          MemoryRequest::MemoryOperation op, bool eop) :
      messageID_(messageCount++),
      payload_(payload),
      channelID_(destination) {
    MemoryMetadata info;
    info.returnChannel = returnChannel;//cmt.getReturnChannel();
    info.checkL2Tags = 1;//cmt.checkL2Tags();
    info.checkL1Tags = 1;//cmt.checkL1Tags();
    info.skipL2 = 0;//cmt.skipL2();
    info.skipL1 = 0;//cmt.skipL1();
    info.opcode = op;
    info.endOfPacket = eop;
    setMetadata(info.flatten());
  }

  // Constructor for messages between L1 and L2 caches.
  // We do not have the destination address at the time of construction -
  // it is provided later after the directory lookup in the miss handling
  // logic. Provide a placeholder here.
  Flit<T>(T payload, ComponentID source, MemoryRequest::MemoryOperation op, bool eop) :
      messageID_(messageCount++),
      payload_(payload),
      channelID_(ChannelID()) {
    MemoryMetadata info;
    info.returnTileX = source.tile.x;
    info.returnTileY = source.tile.y;
    info.returnChannel = source.position - CORES_PER_TILE;
    info.checkL2Tags = 1;
    info.skipL2 = 0;
    info.opcode = op;
    info.endOfPacket = eop;
    setMetadata(info.flatten());
  }

};

#endif /* FLIT_H_ */
