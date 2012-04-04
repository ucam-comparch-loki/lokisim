/*
 * EnergyTraceReader.cpp
 *
 *  Created on: 3 Apr 2012
 *      Author: db434
 */

#include "EnergyTraceReader.h"
#include "EventTypes.h"
#include <fstream>

std::ifstream* EnergyTraceReader::input = NULL;
EventData readEvent;

void EnergyTraceReader::start(const std::string& filename) {
  input = new std::ifstream(filename.c_str(), std::ios_base::binary);
  read();
}

void EnergyTraceReader::stop() {
  input->close();
  delete input;
  input = NULL;
}

void EnergyTraceReader::read() {
  while(input->good())
    readSingleEvent();
}

void EnergyTraceReader::readSingleEvent() {
  // Determine which type of event is next, so we know how much data to read.
  input->read(readEvent.rawData, sizeof(EnergyEvent));

  size_t bytes;
  switch(readEvent.type) {
    case FIFO_READ:
    case FIFO_WRITE:
      bytes = sizeof(FifoEvent); break;

    case REGISTER_READ:
    case REGISTER_WRITE:
      bytes = sizeof(RegisterEvent); break;

    case MEMORY_READ:
    case MEMORY_WRITE:
      bytes = sizeof(MemoryEvent); break;

    case TAG_CHECK:
      bytes = sizeof(TagCheckEvent); break;

    case DECODE:
      bytes = sizeof(DecodeEvent); break;

    case EXECUTE:
      bytes = sizeof(ExecuteEvent); break;

    case DATA_BYPASS:
      bytes = sizeof(BypassEvent); break;

    case NETWORK_TRAFFIC:
      bytes = sizeof(NetworkEvent); break;

    case ARBITRATION:
      bytes = sizeof(ArbitrationEvent); break;

    default:
      bytes = 0;
      std::cerr << "Error: unrecognised energy event code " << (int)readEvent.type << std::endl;
      assert(false);
  }

  // Read the data into the middle of the rawData array, since we've already
  // read the event type.
  input->read(&(readEvent.rawData[sizeof(EnergyEvent)]), bytes);

  // TODO: pass the event to the appropriate model

}
