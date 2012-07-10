/*
 * ChannelMapTable.cpp
 *
 *  Created on: 19 Aug 2011
 *      Author: db434
 */

#include "ChannelMapTable.h"
#include "../../Utility/Instrumentation/ChannelMap.h"

ChannelID ChannelMapTable::read(MapIndex entry) const {
  assert(entry < table.size());

  if (ENERGY_TRACE)
    Instrumentation::ChannelMap::read(previousRead, table[entry]);
  const_cast<ChannelMapTable*>(this)->previousRead = table[entry];

  return table[entry].destination();
}

ChannelMapEntry::NetworkType ChannelMapTable::getNetwork(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].network();
}

void ChannelMapTable::write(MapIndex entry, ChannelID destination,
                            int groupBits, int lineBits) {
  assert(entry < table.size());

  // Wait for all communication with the previous destination to complete
  // before changing the destination. (Why?)
  waitForAllCredits(entry);

  ChannelMapEntry previous = table[entry];

  if(destination.isCore())
    table[entry].setCoreDestination(destination);
  else
    table[entry].setMemoryDestination(destination, groupBits, lineBits);

  if (ENERGY_TRACE)
    Instrumentation::ChannelMap::write(previous, table[entry]);

  if (DEBUG)
    cout << this->name() << " updated map " << (int)entry << " to " << destination << " [" << groupBits << "]" << endl;
}

void ChannelMapTable::waitForAllCredits(MapIndex entry) {
  while(!table[entry].haveAllCredits()) {
    // Call an instrumentation method to say that we have stalled?
    wait(creditArrivedEvent[entry]);
  }
}

void ChannelMapTable::addCredit(MapIndex entry) {
  assert(entry < table.size());
  table[entry].addCredit();
  creditArrivedEvent[entry].notify();
}

void ChannelMapTable::removeCredit(MapIndex entry) {
  assert(entry < table.size());
  table[entry].removeCredit();
}

bool ChannelMapTable::canSend(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].canSend();
}

ChannelMapEntry& ChannelMapTable::getEntry(MapIndex entry) {
  // FIXME: does this method need instrumentation too?
  assert(entry < table.size());
  return table[entry];
}

ChannelMapTable::ChannelMapTable(const sc_module_name& name, ComponentID ID) :
    Component(name, ID),
    table(CHANNEL_MAP_SIZE, ChannelMapEntry(ID)),
    previousRead(ID) {

  creditArrivedEvent.init(CHANNEL_MAP_SIZE);
}
