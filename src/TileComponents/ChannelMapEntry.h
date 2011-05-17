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
  int memoryGroupBits() const;

  bool canSend() const;
  bool haveAllCredits() const;

  void setCoreDestination(const ChannelID& address);
  void setMemoryDestination(const ChannelID& address, uint memoryGroupBits);

  void removeCredit();
  void addCredit();

  ChannelMapEntry();

private:

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
};

#endif /* CHANNELMAPENTRY_H_ */
