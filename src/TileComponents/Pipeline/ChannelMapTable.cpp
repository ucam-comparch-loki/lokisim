/*
 * ChannelMapTable.cpp
 *
 *  Created on: 19 Aug 2011
 *      Author: db434
 */

#include "ChannelMapTable.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Instrumentation/ChannelMap.h"

ChannelID ChannelMapTable::getDestination(MapIndex entry) const {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::read(previousRead, table[entry]);
    const_cast<ChannelMapTable*>(this)->activeCycle();
  }

  const_cast<ChannelMapTable*>(this)->previousRead = table[entry];

  return table[entry].getDestination();
}

ChannelMapEntry::NetworkType ChannelMapTable::getNetwork(MapIndex entry) const {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);
  return table[entry].getNetwork();
}

bool ChannelMapTable::write(MapIndex entry, EncodedCMTEntry data) {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);

  ChannelMapEntry previous = table[entry];
  table[entry].write(data);

  if (table[entry].getDestination().isMemory())
    memoryConnection[table[entry].getReturnChannel()] = true;

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::write(previous, table[entry]);
    activeCycle();
  }

  LOKI_LOG << this->name() << " updated map " << (int)entry << " to " << table[entry].getDestination() << endl;

  return true;
}

EncodedCMTEntry ChannelMapTable::read(MapIndex entry) {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);

  if (ENERGY_TRACE) {
    Instrumentation::ChannelMap::read(previousRead, table[entry]);
    const_cast<ChannelMapTable*>(this)->activeCycle();
  }

  const_cast<ChannelMapTable*>(this)->previousRead = table[entry];

  return table[entry].read();
}

const sc_event& ChannelMapTable::creditArrivedEvent(MapIndex entry) const {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);
  return table[entry].creditArrivedEvent();
}

void ChannelMapTable::addCredit(MapIndex entry, uint numCredits) {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);
  table[entry].addCredit(numCredits);
}

void ChannelMapTable::removeCredit(MapIndex entry) {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);
  table[entry].removeCredit();
}

bool ChannelMapTable::canSend(MapIndex entry) const {
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);
  return table[entry].canSend();
}

bool ChannelMapTable::connectionFromMemory(ChannelIndex channel) const {
  loki_assert_with_message(channel < memoryConnection.size(), "Channel %d", channel);
  return memoryConnection[channel];
}

ChannelMapEntry& ChannelMapTable::operator[] (const MapIndex entry) {
  // FIXME: does this method need instrumentation too?
  loki_assert_with_message(entry < table.size(), "Entry %d", entry);
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
    memoryConnection(CORE_INPUT_CHANNELS, false),
    previousRead(ChannelID(ID, 0)),
    lastActivity(-1) {

  loki_assert(CHANNEL_MAP_SIZE > 0);

  for (uint i=0; i<CHANNEL_MAP_SIZE; i++)
    table.push_back(ChannelID(ID, i));

}
