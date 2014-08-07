/*
 * ChannelMapTable.cpp
 *
 *  Created on: 19 Aug 2011
 *      Author: db434
 */

#include "ChannelMapTable.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/ChannelMap.h"

ChannelID ChannelMapTable::read(MapIndex entry) const {
  assert(entry < table.size());

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::read(previousRead, table[entry]);
    const_cast<ChannelMapTable*>(this)->activeCycle();
  }

  const_cast<ChannelMapTable*>(this)->previousRead = table[entry];

  return table[entry].destination();
}

ChannelMapEntry::NetworkType ChannelMapTable::getNetwork(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].network();
}

bool ChannelMapTable::write(MapIndex entry,
                            ChannelID destination,
                            int groupBits,
                            int lineBits,
                            ChannelIndex returnTo,
                            bool writeThrough) {
  assert(entry < table.size());

  // Wait for all communication with the previous destination to complete
  // before changing the destination. (Why?)
  if (!table[entry].haveAllCredits())
    return false; // Can't complete the write

  ChannelMapEntry previous = table[entry];

  if(destination.isCore())
    table[entry].setCoreDestination(destination);
  else
    table[entry].setMemoryDestination(destination, groupBits, lineBits, returnTo, writeThrough);

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::write(previous, table[entry]);
    activeCycle();
  }

  if (DEBUG)
    cout << this->name() << " updated map " << (int)entry << " to " << destination << " [" << groupBits << "]" << endl;

  return true;
}

const sc_event& ChannelMapTable::haveAllCredits(MapIndex entry) const {
  return allCreditsEvent[entry];
}

void ChannelMapTable::addCredit(MapIndex entry) {
  assert(entry < table.size());
  table[entry].addCredit();
  if (table[entry].haveAllCredits())
    allCreditsEvent[entry].notify();
}

void ChannelMapTable::removeCredit(MapIndex entry) {
  assert(entry < table.size());
  table[entry].removeCredit();
}

bool ChannelMapTable::canSend(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].canSend();
}

ChannelMapEntry& ChannelMapTable::operator[] (const MapIndex entry) {
  // FIXME: does this method need instrumentation too?
  assert(entry < table.size());
  return table[entry];
}

void ChannelMapTable::activeCycle() {
  cycle_count_t currentCycle = Instrumentation::currentCycle();
  if (currentCycle != lastActivity) {
    Instrumentation::ChannelMap::activeCycle();
    lastActivity = currentCycle;
  }
}

ChannelMapTable::ChannelMapTable(const sc_module_name& name, ComponentID ID) :
    Component(name, ID),
    table(CHANNEL_MAP_SIZE, ChannelMapEntry(ID)),
    previousRead(ID),
    lastActivity(-1) {

  allCreditsEvent.init(CHANNEL_MAP_SIZE);
}
