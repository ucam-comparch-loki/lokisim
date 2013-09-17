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

  // Update an entry of the channel map table. This may result in a wait if
  // the entry is still waiting for credits from the previous destination.
  void write(MapIndex entry, ChannelID destination, int groupBits=0, int lineBits=0, ChannelIndex returnTo=0);

  void waitForAllCredits(MapIndex entry);

  void addCredit(MapIndex entry);
  void removeCredit(MapIndex entry);

  bool canSend(MapIndex entry) const;

  // Return an entire entry of the table. This method should be avoided if
  // necessary, as it may bypass functionality in other accessor methods.
  ChannelMapEntry& getEntry(MapIndex entry);

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
  LokiVector<sc_event> creditArrivedEvent;

  // Store a copy of the most recently read entry so we can measure the number
  // of bits which toggle.
  ChannelMapEntry previousRead;
  cycle_count_t lastActivity;

};

#endif /* CHANNELMAPTABLE_H_ */
