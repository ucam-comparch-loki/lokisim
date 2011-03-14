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
#include "../Utility/Parameters.h"

class ChannelMapEntry {

public:

  ChannelID destination() const;
  int network() const;
  bool canSend() const;
  bool haveAllCredits() const;

  void destination(ChannelID address);
  void removeCredit();
  void addCredit();

  ChannelMapEntry& operator=(ChannelID address);

  ChannelMapEntry();

private:

  // The network address to send data to.
  ChannelID destination_;

  // The network to send data on (e.g. core-to-core or core-to-memory).
  unsigned int network_;

  // Whether or not the network being used requires an up-to-date credit counter.
  bool useCredits_;

  // The number of credits this output has.
  unsigned int credits_;

};

#endif /* CHANNELMAPENTRY_H_ */
