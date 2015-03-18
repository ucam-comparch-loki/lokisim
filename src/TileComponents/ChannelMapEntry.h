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

// https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/architecture/core/channel_map_table/
//
// Format:
//  line size:        2 bits encoded - options in MemoryTypedefs.h
//  log2(group size): 2 bits
//  return channel:   3 bits
//  tile:             6 bits (x:3, y:3)
//  core/bank:        3 bits
//  multicast?:       1 bit
//  channel:          3 bits
typedef unsigned int EncodedCMTEntry;

class ChannelMapEntry {

private:

  static const uint CHANNEL_WIDTH         = 3;
  static const uint MULTICAST_WIDTH       = 1;
  static const uint POSITION_WIDTH        = 3;
  static const uint TILE_WIDTH            = 6;
  static const uint TILE_X_WIDTH          = 3;
  static const uint TILE_Y_WIDTH          = 3;
  static const uint COREMASK_WIDTH        = 8;
  static const uint RETURN_CHANNEL_WIDTH  = 3;
  static const uint GROUP_SIZE_WIDTH      = 2;
  static const uint LINE_SIZE_WIDTH       = 2;

  static const uint CHANNEL_START         = 0;
  static const uint MULTICAST_START       = CHANNEL_START        + CHANNEL_WIDTH;
  static const uint POSITION_START        = MULTICAST_START      + MULTICAST_WIDTH;
  static const uint TILE_START            = POSITION_START       + POSITION_WIDTH;
  static const uint TILE_Y_START          = TILE_START;
  static const uint TILE_X_START          = TILE_Y_START         + TILE_Y_WIDTH;
  static const uint COREMASK_START        = POSITION_START;
  static const uint RETURN_CHANNEL_START  = TILE_START           + TILE_WIDTH;
  static const uint GROUP_SIZE_START      = RETURN_CHANNEL_START + RETURN_CHANNEL_WIDTH;
  static const uint LINE_SIZE_START       = GROUP_SIZE_START     + GROUP_SIZE_WIDTH;

public:

  enum NetworkType {CORE_TO_CORE, CORE_TO_MEMORY, GLOBAL};

  ChannelID getDestination() const;
  NetworkType getNetwork() const;
  bool writeThrough() const;

  bool canSend() const;
  bool haveAllCredits() const;

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

  void removeCredit();
  void addCredit();

  bool usesCredits() const;

  uint popCount() const;
  uint hammingDistance(const ChannelMapEntry& other) const;

  // Event which is triggered when a channel's credit counter reaches its
  // maximum value.
  const sc_event& allCreditsEvent() const;

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

  // Whether or not the network being used requires an up-to-date credit counter.
  bool useCredits_;

  // The number of credits this output has.
  unsigned int credits_;

  // Whether or not this channel accesses the memory in write-through mode.
  bool writeThrough_;

  // The current address increment for this entry.
  unsigned int addressIncrement_;

  // Event triggered whenever a credit arrives.
  sc_event creditArrived_;

  // Event triggered whenever the credit counter reaches its maximum.
  sc_event haveAllCredits_;
};

#endif /* CHANNELMAPENTRY_H_ */
