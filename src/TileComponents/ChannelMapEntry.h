/*
 * ChannelMapEntry.h
 *
 *  Created on: 14 Mar 2011
 *      Author: db434
 */

#ifndef CHANNELMAPENTRY_H_
#define CHANNELMAPENTRY_H_

#include <assert.h>
#include "../Typedefs.h"
#include "../Datatype/ChannelID.h"
#include "../Utility/Parameters.h"

class ChannelMapEntry {

public:

  enum NetworkType {CORE_TO_CORE, CORE_TO_MEMORY, GLOBAL};

  ChannelID destination() const;
  NetworkType network() const;
  bool localMemory() const;
  uint memoryGroupBits() const;
  uint memoryLineBits() const;

  bool canSend() const;
  bool haveAllCredits() const;

  void setCoreDestination(const ChannelID& address);
  void setMemoryDestination(const ChannelID& address, uint memoryGroupBits, uint memoryLineBits);

  void setAddressIncrement(uint increment);
  uint getAddressIncrement() const;

  void removeCredit();
  void addCredit();

  bool usesCredits() const;

  uint popCount() const;
  uint hammingDistance(const ChannelMapEntry& other) const;

  ChannelMapEntry(ComponentID localID);

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

  // Number of group bits describing virtual memory bank.
  unsigned int memoryGroupBits_;

  // Number of line bits describing virtual memory bank.
  unsigned int memoryLineBits_;

  // The current address increment for this entry.
  unsigned int addressIncrement_;
};

#endif /* CHANNELMAPENTRY_H_ */
