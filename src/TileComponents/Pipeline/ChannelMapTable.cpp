/*
 * ChannelMapTable.cpp
 *
 *  Created on: 19 Aug 2011
 *      Author: db434
 */

#include "ChannelMapTable.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/ChannelMap.h"

ChannelID ChannelMapTable::getDestination(MapIndex entry) const {
  assert(entry < table.size());

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::read(previousRead, table[entry]);
    const_cast<ChannelMapTable*>(this)->activeCycle();
  }

  const_cast<ChannelMapTable*>(this)->previousRead = table[entry];

  return table[entry].getDestination();
}

ChannelMapEntry::NetworkType ChannelMapTable::getNetwork(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].getNetwork();
}

bool ChannelMapTable::write(MapIndex entry, EncodedCMTEntry data) {
  assert(entry < table.size());

  // Wait for all communication with the previous destination to complete
  // before changing the destination. (Why?)
  if (!table[entry].haveAllCredits())
    return false; // Can't complete the write

  ChannelMapEntry previous = table[entry];
  table[entry].write(data);

  if (table[entry].getDestination().isMemory())
    memoryConnection[table[entry].getReturnChannel()] = true;

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::write(previous, table[entry]);
    activeCycle();
  }

  if (DEBUG)
    cout << this->name() << " updated map " << (int)entry << " to " << table[entry].getDestination() << endl;

  return true;
}

EncodedCMTEntry ChannelMapTable::read(MapIndex entry) {
  assert(entry < table.size());

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::read(previousRead, table[entry]);
    const_cast<ChannelMapTable*>(this)->activeCycle();
  }

  const_cast<ChannelMapTable*>(this)->previousRead = table[entry];

  return table[entry].read();
}

const sc_event& ChannelMapTable::allCreditsEvent(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].allCreditsEvent();
}

const sc_event& ChannelMapTable::creditArrivedEvent(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].creditArrivedEvent();
}

void ChannelMapTable::addCredit(MapIndex entry) {
  assert(entry < table.size());
  table[entry].addCredit();
}

void ChannelMapTable::removeCredit(MapIndex entry) {
  assert(entry < table.size());
  table[entry].removeCredit();
}

bool ChannelMapTable::canSend(MapIndex entry) const {
  assert(entry < table.size());
  return table[entry].canSend();
}

bool ChannelMapTable::connectionFromMemory(ChannelIndex channel) const {
  assert(channel < memoryConnection.size());
  return memoryConnection[channel];
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
    memoryConnection(CORE_INPUT_CHANNELS, false),
    previousRead(ID),
    lastActivity(-1) {
  // Do nothing.
}
