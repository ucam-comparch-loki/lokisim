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
#include "../Typedefs.h"
#include "../Datatype/ChannelID.h"
#include "../Utility/Parameters.h"

using sc_core::sc_event;

class ChannelMapEntry {

public:

  enum NetworkType {CORE_TO_CORE, CORE_TO_MEMORY, GLOBAL};

  ChannelID destination() const;
  NetworkType network() const;
  bool localMemory() const;
  uint memoryGroupBits() const;
  uint memoryLineBits() const;
  bool writeThrough() const;
  ChannelIndex returnChannel() const;

  bool canSend() const;
  bool haveAllCredits() const;

  void setCoreDestination(const ChannelID& address);
  void setMemoryDestination(const ChannelID& address, uint memoryGroupBits,
                            uint memoryLineBits, ChannelIndex returnTo,
                            bool writeThrough = false);

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

  ChannelMapEntry(ComponentID localID);
  ChannelMapEntry(const ChannelMapEntry& other);
  ChannelMapEntry& operator=(const ChannelMapEntry& other);

private:

  // The ID of the component holding this channel map entry. Used to determine
  // whether communications are local or global.
  ComponentID id_;

  // The network address to send data to.
  ChannelID destination_;

  // The network to send data on (e.g. core-to-core or core-to-memory).
  NetworkType network_;

  // Whether or not the network being used requires an up-to-date credit counter.
  bool useCredits_;

  // The number of credits this output has.
  unsigned int credits_;

  // Whether or not this is a local memory bank.
  bool localMemory_;

  // The input channel of this core that memory banks should send data back to.
  ChannelIndex returnChannel_;

  // Number of group bits describing virtual memory bank.
  unsigned int memoryGroupBits_;

  // Number of line bits describing virtual memory bank.
  unsigned int memoryLineBits_;

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
