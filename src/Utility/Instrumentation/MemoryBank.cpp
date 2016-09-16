/*
 * MemoryBank.cpp
 *
 *  Created on: 19 Apr 2016
 *      Author: db434
 */

#include "../Parameters.h"
#include "MemoryBank.h"

#include <map>

#include "../../Exceptions/InvalidOptionException.h"
#include "IPKCache.h"

using namespace Instrumentation;
using std::vector;

CounterMap<int> MemoryBank::tagChecks;

vector<CounterMap<int> > MemoryBank::hits;
vector<CounterMap<int> > MemoryBank::misses;

CounterMap<int> MemoryBank::ipkReads;
CounterMap<int> MemoryBank::burstReads;
CounterMap<int> MemoryBank::burstWrites;

CounterMap<int> MemoryBank::replaceInvalidLine;
CounterMap<int> MemoryBank::replaceCleanLine;
CounterMap<int> MemoryBank::replaceDirtyLine;

vector<vector<struct MemoryBank::ChannelStats> > MemoryBank::coreStats;

void MemoryBank::init() {
  // Initial stats for one channel of one core.
  struct ChannelStats nullData;
  nullData.readHits = 0;
  nullData.readMisses = 0;
  nullData.writeHits = 0;
  nullData.writeMisses = 0;
  nullData.receivingInstructions = false;

  // Initial stats for all channels of one core.
  vector<struct ChannelStats> oneCore;
  oneCore.assign(CORE_INPUT_CHANNELS, nullData);

  // Initial stats for all channels of all cores.
  coreStats.assign(NUM_CORES, oneCore);

  // Clean all the CounterMaps.
  tagChecks.clear();

  hits.clear();
  misses.clear();
  for (int i=0; i<PAYLOAD_EOP; i++) {
    hits.push_back(CounterMap<int>());
    misses.push_back(CounterMap<int>());
  }

  ipkReads.clear();
  burstReads.clear();
  burstWrites.clear();
  replaceInvalidLine.clear();
  replaceCleanLine.clear();
  replaceDirtyLine.clear();
}

void MemoryBank::startOperation(int bank, MemoryOpcode op,
        MemoryAddr address, bool miss, ChannelID returnChannel) {
  switch (op) {
    // "Normal" operations which complete after the first memory access.
    case LOAD_W:
    case LOAD_LINKED:
    case LOAD_HW:
    case LOAD_B:
    case STORE_W:
    case STORE_CONDITIONAL:
    case STORE_HW:
    case STORE_B:
    case LOAD_AND_ADD:
    case LOAD_AND_OR:
    case LOAD_AND_AND:
    case LOAD_AND_XOR:
    case EXCHANGE:
    case UPDATE_DIRECTORY_ENTRY:
    case UPDATE_DIRECTORY_MASK:
      if (miss)
        misses[op].increment(bank);
      else
        hits[op].increment(bank);

      updateCoreStats(returnChannel, op, miss);
      break;

    // Cache line operations just notify when they begin, and report
    // separately for each word they access.
    case FETCH_LINE:
    case FLUSH_LINE:
      burstReads.increment(bank);
      break;
    case IPK_READ:
      ipkReads.increment(bank);
      break;
    case STORE_LINE:
    case MEMSET_LINE:
    case PUSH_LINE:
      burstWrites.increment(bank);
      break;

    // Some operations do not have any significant effect on energy since they
    // act mainly on the metadata.
    case VALIDATE_LINE:
    case PREFETCH_LINE:
    case INVALIDATE_LINE:
    case FLUSH_ALL_LINES:
    case INVALIDATE_ALL_LINES:
      break;

    default:
      throw InvalidOptionException("memory opcode (instrumentation)", op);
      break;
  }
}

void MemoryBank::continueOperation(int bank, MemoryOpcode op,
        MemoryAddr address, bool miss, ChannelID returnChannel) {
  switch (op) {
    // Only some of the cache line operations do anything interesting when
    // they are being continued.
    case FETCH_LINE:
    case FLUSH_LINE:
    case IPK_READ:
    case STORE_LINE:
    case MEMSET_LINE:
    case PUSH_LINE:
      if (miss)
        misses[op].increment(bank);
      else
        hits[op].increment(bank);

      updateCoreStats(returnChannel, op, miss);
      break;

    default:
      break;
  }
}

void MemoryBank::checkTags(int bank, MemoryAddr address) {
  tagChecks.increment(bank);

  // Do something with Hamming distance of address?
}

void MemoryBank::replaceCacheLine(int bank, bool isValid, bool isDirty) {
  if (!isValid)
    replaceInvalidLine.increment(bank);
  else if (isDirty)
    replaceDirtyLine.increment(bank);
  else
    replaceCleanLine.increment(bank);
}

void MemoryBank::updateCoreStats(ChannelID returnChannel, MemoryOpcode op, bool miss) {
  if (!returnChannel.component.isCore())
    return;

  bool instruction = (op == IPK_READ);
  bool write;

  switch (op) {
    case STORE_W:
    case STORE_CONDITIONAL:
    case STORE_HW:
    case STORE_B:
    case STORE_LINE:
    case MEMSET_LINE:
    case PUSH_LINE:
    case LOAD_AND_ADD:
    case LOAD_AND_OR:
    case LOAD_AND_AND:
    case LOAD_AND_XOR:
    case EXCHANGE:
      write = true;
      break;

    default:
      write = false;
      break;
  }

  uint core = returnChannel.component.globalCoreNumber();
  uint channel = returnChannel.channel;

  if (write && miss)
    coreStats[core][channel].writeMisses++;
  else if (write && !miss)
    coreStats[core][channel].writeHits++;
  else if (!write && miss)
    coreStats[core][channel].readMisses++;
  else if (!write && !miss)
    coreStats[core][channel].readHits++;
  coreStats[core][channel].receivingInstructions = instruction;
}

void MemoryBank::printSummary() {
  count_t instructionReadHits = hits[IPK_READ].numEvents();
  count_t instructionReads = instructionReadHits + misses[IPK_READ].numEvents();
  count_t l0l1InstReads = IPKCache::numReads();
  count_t l0l1InstReadHits = l0l1InstReads - misses[IPK_READ].numEvents();
  count_t atomicHits = hits[LOAD_AND_ADD].numEvents() + hits[LOAD_AND_AND].numEvents() + hits[LOAD_AND_OR].numEvents() + hits[LOAD_AND_XOR].numEvents() + hits[EXCHANGE].numEvents();
  count_t atomicMisses = misses[LOAD_AND_ADD].numEvents() + misses[LOAD_AND_AND].numEvents() + misses[LOAD_AND_OR].numEvents() + misses[LOAD_AND_XOR].numEvents() + misses[EXCHANGE].numEvents();
  count_t dataReadHits = atomicHits + hits[LOAD_B].numEvents() + hits[LOAD_HW].numEvents() + hits[LOAD_W].numEvents() + hits[LOAD_LINKED].numEvents();
  count_t dataReads = dataReadHits + atomicMisses + misses[LOAD_B].numEvents() + misses[LOAD_HW].numEvents() + misses[LOAD_W].numEvents() + misses[LOAD_LINKED].numEvents();
  count_t dataWriteHits = atomicHits + hits[STORE_B].numEvents() + hits[STORE_HW].numEvents() + hits[STORE_W].numEvents() + hits[STORE_CONDITIONAL].numEvents();
  count_t dataWrites = dataWriteHits + atomicMisses + misses[STORE_B].numEvents() + misses[STORE_HW].numEvents() + misses[STORE_W].numEvents() + misses[STORE_CONDITIONAL].numEvents();
  count_t totalHits = instructionReadHits + dataReadHits + dataWriteHits - atomicHits;
  count_t totalAccesses = instructionReads + dataReads + dataWrites - atomicHits - atomicMisses;

  using std::clog;

  // Note that for every miss event, there will be a corresponding hit event
  // when the data arrives. We may prefer to filter out these "duplicates".
  clog << "L1 cache activity:\n";
  clog << "  By channel:\n";

  for (uint core=0; core<coreStats.size(); core++) {
    for (uint channel=0; channel<coreStats[core].size(); channel++) {
      ChannelStats stats = coreStats[core][channel];
      if (stats.readHits > 0 || stats.readMisses > 0 || stats.writeHits > 0 || stats.writeMisses > 0) {
        clog << "    Core " << core << ", input channel " << channel << (stats.receivingInstructions ? " (instructions)" : " (data)") << ":\n";
        clog << "      Read hits:  " << stats.readHits << "/" << (stats.readHits+stats.readMisses) << " (" << percentage(stats.readHits, stats.readHits+stats.readMisses) << ")\n";
        clog << "      Write hits: " << stats.writeHits << "/" << (stats.writeHits+stats.writeMisses) << " (" << percentage(stats.writeHits, stats.writeHits+stats.writeMisses) << ")\n";
        clog << "      Total hits: " << stats.readHits+stats.writeHits << "/" << (stats.readHits+stats.readMisses+stats.writeHits+stats.writeMisses) << " (" << percentage(stats.readHits+stats.writeHits, stats.readHits+stats.readMisses+stats.writeHits+stats.writeMisses) << ")\n";
      }
    }
  }

  clog << "  Instruction hits: " << instructionReadHits << "/" << instructionReads << " (" << percentage(instructionReadHits, instructionReads) << ")\n";
  clog << "  L0+L1 inst hits:  " << l0l1InstReadHits << "/" << l0l1InstReads << " (" << percentage(l0l1InstReadHits,l0l1InstReads) << ")\n";
  clog << "  Data read hits:   " << dataReadHits << "/" << dataReads << " (" << percentage(dataReadHits, dataReads) << ")\n";
  clog << "  Data write hits:  " << dataWriteHits << "/" << dataWrites << " (" << percentage(dataWriteHits, dataWrites) << ")\n";
  clog << "  Total hits:       " << totalHits << "/" << totalAccesses << " (" << percentage(totalHits, totalAccesses) << ")\n";
}

void MemoryBank::dumpEventCounts(std::ostream& os) {
  count_t ipkReads = hits[IPK_READ].numEvents() + misses[IPK_READ].numEvents();

  os << "<memory size=\"" << MEMORY_BANK_SIZE                    << "\">\n"
     << xmlNode("instances",      NUM_MEMORIES)                  << "\n"
     << xmlNode("active",         totalReads() + totalWrites())  << "\n"
     << xmlNode("read",           totalReads())                  << "\n"
     << xmlNode("write",          totalWrites())                 << "\n"
     << xmlNode("tag_check",      tagChecks.numEvents())         << "\n"
     << xmlNode("word_read",      totalWordReads())              << "\n"
     << xmlNode("halfword_read",  totalHalfwordReads())          << "\n"
     << xmlNode("byte_read",      totalByteReads())              << "\n"
     << xmlNode("ipk_read",       ipkReads)                      << "\n"
     << xmlNode("burst_read",     totalBurstReads() - ipkReads)  << "\n"
     << xmlNode("word_write",     totalWordWrites())             << "\n"
     << xmlNode("halfword_write", totalHalfwordWrites())         << "\n"
     << xmlNode("byte_write",     totalByteWrites())             << "\n"
     << xmlNode("burst_write",    totalBurstWrites())            << "\n"
     << xmlNode("replace_line",   totalLineReplacements())       << "\n"
     << xmlEnd("memory")                                         << "\n";
}

count_t MemoryBank::totalReads() {
  return totalWordReads()
       + totalHalfwordReads()
       + totalByteReads()
       + totalBurstReads();
}

count_t MemoryBank::totalWrites() {
  return totalWordWrites()
       + totalHalfwordWrites()
       + totalByteWrites()
       + totalBurstWrites();
}

count_t MemoryBank::totalWordReads() {
  // Include atomic memory operations too?
  return hits[LOAD_W].numEvents()            + misses[LOAD_W].numEvents()
       + hits[LOAD_LINKED].numEvents()       + misses[LOAD_LINKED].numEvents();
}

count_t MemoryBank::totalHalfwordReads() {
  return hits[LOAD_HW].numEvents()           + misses[LOAD_HW].numEvents();
}

count_t MemoryBank::totalByteReads() {
  return hits[LOAD_B].numEvents()            + misses[LOAD_B].numEvents();
}

count_t MemoryBank::totalBurstReads() {
  return hits[FETCH_LINE].numEvents()        + misses[FETCH_LINE].numEvents()
       + hits[IPK_READ].numEvents()          + misses[IPK_READ].numEvents()
       + hits[FLUSH_LINE].numEvents()        + misses[FLUSH_LINE].numEvents();
}

count_t MemoryBank::totalWordWrites() {
  return hits[STORE_W].numEvents()           + misses[STORE_W].numEvents()
       + hits[STORE_CONDITIONAL].numEvents() + misses[STORE_CONDITIONAL].numEvents()
       + hits[LOAD_AND_ADD].numEvents()      + misses[LOAD_AND_ADD].numEvents()
       + hits[LOAD_AND_OR].numEvents()       + misses[LOAD_AND_OR].numEvents()
       + hits[LOAD_AND_AND].numEvents()      + misses[LOAD_AND_AND].numEvents()
       + hits[LOAD_AND_XOR].numEvents()      + misses[LOAD_AND_XOR].numEvents()
       + hits[EXCHANGE].numEvents()          + misses[EXCHANGE].numEvents();
}

count_t MemoryBank::totalHalfwordWrites() {
  return hits[STORE_HW].numEvents()          + misses[STORE_HW].numEvents();
}

count_t MemoryBank::totalByteWrites() {
  return hits[STORE_B].numEvents()           + misses[STORE_B].numEvents();
}

count_t MemoryBank::totalBurstWrites() {
  return hits[STORE_LINE].numEvents()        + misses[STORE_LINE].numEvents()
       + hits[MEMSET_LINE].numEvents()       + misses[MEMSET_LINE].numEvents()
       + hits[PUSH_LINE].numEvents()         + misses[PUSH_LINE].numEvents();
}

count_t MemoryBank::totalLineReplacements() {
  return replaceInvalidLine.numEvents() + replaceCleanLine.numEvents()
       + replaceDirtyLine.numEvents();
}

count_t MemoryBank::numIPKReadMisses() {
  return misses[IPK_READ].numEvents();
}
