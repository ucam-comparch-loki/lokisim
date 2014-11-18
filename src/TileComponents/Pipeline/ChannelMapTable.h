/*
 * ChannelMapTable.h
 *
 *  Created on: 19 Aug 2011
 *      Author: db434
 */

#ifndef CHANNELMAPTABLE_H_
#define CHANNELMAPTABLE_H_

#include "../../Component.h"
#include "../ChannelMapEntry.h"

class ChannelMapTable : public Component {

//==============================//
// Methods
//==============================//

public:

  // Determine where to send data to if it specifies this channel entry.
  ChannelID read(MapIndex entry) const;
  ChannelMapEntry::NetworkType getNetwork(MapIndex entry) const;

  // Update an entry of the channel map table. Returns whether the update was
  // successful. If not, the update will need to be reattempted when all
  // credits have arrived.
  bool write(MapIndex entry,
             ChannelID destination,
             int groupBits=0,
             int lineBits=0,
             ChannelIndex returnTo=0,
             bool writeThrough=false);

  // Event which is triggered when a channel's credit counter reaches its
  // maximum value.
  const sc_event& allCreditsEvent(MapIndex entry) const;

  // Event triggered when a credit arrives for a particular channel.
  const sc_event& creditArrivedEvent(MapIndex entry) const;

  void addCredit(MapIndex entry);
  void removeCredit(MapIndex entry);

  bool canSend(MapIndex entry) const;

  // Return an entire entry of the table. This method should be avoided if
  // necessary, as it may bypass functionality in other accessor methods.
  ChannelMapEntry& operator[] (const MapIndex entry);

  // For instrumentation purposes only, keep track of which of our input
  // channels we expect to receive data from memory, and which will receive
  // data from cores. ChannelIndex 0 is mapped to the IPK FIFO.
  bool connectionFromMemory(ChannelIndex channel) const;

private:

  // Keep track of the number of cycles this component is active so we can
  // estimate the benefits of clock gating.
  void activeCycle();

//==============================//
// Constructors and destructors
//==============================//

public:

  ChannelMapTable(const sc_module_name& name, ComponentID ID);

//==============================//
// Local state
//==============================//

private:

  std::vector<ChannelMapEntry> table;

  // One entry per input channel. true = we've set up a memory connection to
  // return data to this input. false = anything else.
  std::vector<bool> memoryConnection;

  // Store a copy of the most recently read entry so we can measure the number
  // of bits which toggle.
  ChannelMapEntry previousRead;
  cycle_count_t lastActivity;

};

#endif /* CHANNELMAPTABLE_H_ */
