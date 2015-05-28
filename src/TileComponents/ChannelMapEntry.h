/*
 * ChannelMapEntry.h
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#ifndef CHANNELMAPENTRY_H_
#define CHANNELMAPENTRY_H_

#include <assert.h>
#include "systemc"
#include "Memory/MemoryTypedefs.h"
#include "../Typedefs.h"
#include "../Datatype/Identifier.h"
#include "../Utility/Parameters.h"

using sc_core::sc_event;

// Full documentation at:
// https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/architecture/core/channel_map_table/
//
// Core-multicast:
//                                  |psm|   core mask   |  ch |0|0|
//
// Core-to-L1:
//  | L2tags | L1tags | L2skip | L1skip |grp|retCh| bank|  ch |1|0|
//
// Core-to-core global:
//  |we|                |  credits  |tileX|tileY| core|  ch |acq|1|
typedef unsigned int EncodedCMTEntry;

class ChannelMapEntry {

public:

  struct MulticastChannel {
    uint padding           : 18;
    uint pipelineStallMode : 1;
    uint coreMask          : 8;
    uint channel           : 3;
    uint isMemory          : 1;
    uint isGlobal          : 1;

    uint flatten() {
      return (padding << 14) | (pipelineStallMode << 13) | (coreMask << 5) |
             (channel << 2) | (isMemory << 1) | (isGlobal << 0);
    }

    MulticastChannel(uint flat) {
      padding           = (flat >> 14) & 0x3FFFF;
      pipelineStallMode = (flat >> 13) & 0x1;
      coreMask          = (flat >> 5) & 0xFF;
      channel           = (flat >> 2) & 0x7;
      isMemory          = (flat >> 1) & 0x1;
      isGlobal          = (flat >> 0) & 0x1;
    }
  };

  struct MemoryChannel {
    uint padding           : 15;
    uint l2TagCheck        : 1;
    uint l1TagCheck        : 1;
    uint l2Skip            : 1;
    uint l1Skip            : 1;
    uint groupSize         : 2;
    uint returnChannel     : 3;
    uint bank              : 3;
    uint channel           : 3;
    uint isMemory          : 1;
    uint isGlobal          : 1;

    uint flatten() {
      return (padding << 17) | (l2TagCheck << 16) | (l1TagCheck << 15) |
             (l2Skip << 14) | (l1Skip << 13) | (groupSize << 11) |
             (returnChannel << 8) | (bank << 5) | (channel << 2) |
             (isMemory << 1) | (isGlobal << 0);
    }

    MemoryChannel(uint flat) {
      padding       = (flat >> 17) & 0x7FFF;
      l2TagCheck    = (flat >> 16) & 0x1;
      l1TagCheck    = (flat >> 15) & 0x1;
      l2Skip        = (flat >> 14) & 0x1;
      l1Skip        = (flat >> 13) & 0x1;
      groupSize     = (flat >> 11) & 0x3;
      returnChannel = (flat >> 8) & 0x7;
      bank          = (flat >> 5) & 0x7;
      channel       = (flat >> 2) & 0x7;
      isMemory      = (flat >> 1) & 0x1;
      isGlobal      = (flat >> 0) & 0x1;
    }
  };

  struct GlobalChannel {
    uint padding           : 11;
    uint creditWriteEnable : 1;   // Not actually stored
    uint credits           : 6;
    uint tileX             : 3;
    uint tileY             : 3;
    uint core              : 3;
    uint channel           : 3;
    uint acquired          : 1;
    uint isGlobal          : 1;

    uint flatten() {
      return (padding << 21) | (creditWriteEnable << 20) | (credits << 14) |
             (tileX << 11) | (tileY << 8) | (core << 5) | (channel << 2) |
             (acquired << 1) | (isGlobal << 0);
    }

    GlobalChannel(uint flat) {
      padding           = (flat >> 21) & 0x7FF;
      creditWriteEnable = (flat >> 20) & 0x1;
      credits           = (flat >> 14) & 0x3F;
      tileX             = (flat >> 11) & 0x7;
      tileY             = (flat >> 8) & 0x7;
      core              = (flat >> 5) & 0x7;
      channel           = (flat >> 2) & 0x7;
      acquired          = (flat >> 1) & 0x1;
      isGlobal          = (flat >> 0) & 0x1;
    }
  };

  enum NetworkType {MULTICAST, CORE_TO_MEMORY, GLOBAL, NONE};

  // Access different views of the underlying data.
  static const MulticastChannel multicastView(EncodedCMTEntry data);
  static const MemoryChannel    memoryView(EncodedCMTEntry data);
  static const GlobalChannel    globalView(EncodedCMTEntry data);
  const MulticastChannel multicastView() const;
  const MemoryChannel    memoryView()    const;
  const GlobalChannel    globalView()    const;

  ChannelID getDestination() const;
  NetworkType getNetwork() const;
  bool useCredits() const;
  bool writeThrough() const;

  bool canSend() const;
  bool haveNCredits(uint n) const;
  void removeCredit();
  void addCredit();
  void setCredits(uint count);
  void clearWriteEnable();

  // Write to the channel map entry.
  void write(EncodedCMTEntry data);

  // Get the entire contents of the channel map entry (used for the getchmap
  // instruction).
  EncodedCMTEntry read() const;

  // The tile of the target component.
  unsigned int getTileColumn() const;
  unsigned int getTileRow() const;

  // The position of the target component within its tile.
  ComponentIndex getComponent() const;

  // The channel of the target component to access.
  ChannelIndex getChannel() const;

  // Whether this is a multicast communication.
  bool isMulticast() const;

  // A bitmask of cores in the local tile to communicate with. The same channel
  // on all cores is used.
  unsigned int getCoreMask() const;

  // The number of memory banks in the virtual memory group.
  unsigned int getMemoryGroupSize() const;

  // The number of words in each cache line.
  unsigned int getLineSize() const;

  // The input channel on this core to which any requested data should be sent
  // from memory.
  ChannelIndex getReturnChannel() const;

  // Compute the increment which must be added to the ChannelID of the first
  // memory of a group in order to access the given memory address.
  uint computeAddressIncrement(MemoryAddr address) const;

  // Set the address increment, ready to be used by subsequent flits in the
  // packet.
  void setAddressIncrement(uint increment);

  // Retrieve a precomputed address increment, to ensure that the correct
  // memory bank is accessed.
  uint getAddressIncrement() const;

  uint popCount() const;
  uint hammingDistance(const ChannelMapEntry& other) const;

  // Event triggered when a credit arrives for a particular channel.
  const sc_event& creditArrivedEvent() const;

  ChannelMapEntry(ChannelID localID);
  ChannelMapEntry(const ChannelMapEntry& other);
  ChannelMapEntry& operator=(const ChannelMapEntry& other);

private:

  // The global address of this channel map entry. Used to determine whether
  // communications are local or global.
  ChannelID id_;

  // Encoded data representing a connection to another component.
  EncodedCMTEntry data_;

  // The network to send data on (e.g. core-to-core or core-to-memory).
  NetworkType network_;

  // The current address increment for this entry.
  unsigned int addressIncrement_;

  // Event triggered whenever a credit arrives.
  sc_event creditArrived_;
};

#endif /* CHANNELMAPENTRY_H_ */
