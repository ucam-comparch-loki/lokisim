/*
 * EventTypes.h
 *
 *  Created on: 2 Apr 2012
 *      Author: db434
 */

#ifndef EVENTTYPES_H_
#define EVENTTYPES_H_

#include "../InstructionMap.h"
#include "../../Typedefs.h"

enum EnergyEvent {
  FIFO_READ,
  FIFO_WRITE,
  REGISTER_READ,
  REGISTER_WRITE,
  MEMORY_READ,
  MEMORY_WRITE,
  TAG_CHECK,
  DECODE,
  EXECUTE,
  DATA_BYPASS,
  NETWORK_TRAFFIC,
  ARBITRATION
};

struct FifoEvent {
  uint8_t fifoSize;
  // Do we also want to know which FIFO it is, and what data was read/written?
};

struct RegisterEvent {
  uint8_t tile;
  uint8_t core;
  RegisterIndex reg;
  bool indirect;
};

struct MemoryEvent {
  uint32_t memSizeInBytes;
};

struct TagCheckEvent {
  uint32_t numTags;
  uint8_t tagWidth;
};

struct DecodeEvent {
  opcode_t operation;
};

struct ExecuteEvent {
  opcode_t operation;
  function_t function;
  uint32_t operand1;
  uint32_t operand2;
};

struct BypassEvent {
  uint8_t tile;
  uint8_t core;
  int32_t data;
};

struct NetworkEvent {
  uint8_t tileSource;
  uint8_t componentSource;
  uint8_t tileDestination;
  uint8_t componentDestination;
  uint8_t channelDestination;
  uint32_t data;
  // multicast? uses same energy as point-to-point, so might not matter
};

struct ArbitrationEvent {
  uint8_t inputs;
};

// Can interpret data as a raw byte sequence, or as an event type plus
// information specific to that event.
union EventData {
  char rawData[16];

  struct {
    EnergyEvent type;

    union {
      ArbitrationEvent  arbitrationEvent;
      BypassEvent       bypassEvent;
      DecodeEvent       decodeEvent;
      ExecuteEvent      executeEvent;
      FifoEvent         fifoEvent;
      MemoryEvent       memoryEvent;
      NetworkEvent      networkEvent;
      RegisterEvent     registerEvent;
      TagCheckEvent     tagCheckEvent;
    };
  };
};

#endif /* EVENTTYPES_H_ */
