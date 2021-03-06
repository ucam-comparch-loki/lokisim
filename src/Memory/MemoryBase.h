/*
 * MemoryBase.h
 *
 * The software interface between MemoryOperations and simulated memory
 * structures. This allows MemoryBanks, MainMemory and any other memory-like
 * components to be treated identically.
 *
 *  Created on: 21 Apr 2016
 *      Author: db434
 */

#ifndef SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBASE_H_
#define SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBASE_H_

#include "../Datatype/Instruction.h"
#include "../LokiComponent.h"
#include "../Network/BandwidthMonitor.h"
#include "../Network/NetworkTypes.h"
#include "../Utility/Assert.h"
#include "../Utility/Warnings.h"
#include "MemoryTypes.h"

class MemoryOperation;

class MemoryBase : public LokiComponent {

public:

  MemoryBase(sc_module_name name, ComponentID id, size_t log2CacheLineSize,
             bandwidth_t maxBandwidth=1000) :
    LokiComponent(name),
    id(id),
    log2CacheLineSize(log2CacheLineSize),
    bandwidth(maxBandwidth) {

  }

  virtual ~MemoryBase() {}

  // Return the position in SRAM that the given memory address is to be found.
  virtual SRAMAddress getPosition(
      MemoryAddr       address,
      MemoryAccessMode mode
  ) const = 0;

  // Return the position in the memory address space of the data stored at the
  // given position.
  // TODO: should probably include MemoryAccessMode as a parameter, but don't
  // always have this information.
  virtual MemoryAddr getAddress(
      SRAMAddress      position
  ) const = 0;

  // Return whether data from `address` can be found at `position` in the SRAM.
  virtual bool contains(
      MemoryAddr       address,
      SRAMAddress      position,
      MemoryAccessMode mode
  ) const = 0;

  // Ensure that a valid copy of data from `address` can be found at `position`.
  virtual void allocate(
      MemoryAddr       address,
      SRAMAddress      position,
      MemoryAccessMode mode
  ) = 0;

  // Ensure that there is a space to write data to `address` at `position`.
  virtual void validate(
      MemoryAddr       address,
      SRAMAddress      position,
      MemoryAccessMode mode
  ) = 0;

  // Invalidate the cache line which contains `position`.
  virtual void invalidate(
      SRAMAddress      position,
      MemoryAccessMode mode
  ) = 0;

  // Flush the cache line containing `position` down the memory hierarchy, if
  // necessary. The line is not invalidated, but is no longer dirty.
  virtual void flush(
      SRAMAddress      position,
      MemoryAccessMode mode
  ) = 0;

  // Return whether a payload flit is available. `level` tells whether this bank
  // is being treated as an L1 or L2 cache or main memory.
  virtual bool payloadAvailable(MemoryLevel level) const = 0;

  // Retrieve a payload flit. `level` tells whether this bank is being treated
  // as an L1 or L2 cache or main memory.
  virtual uint32_t getPayload(MemoryLevel level) = 0;

  // Send a result to the requested destination.
  virtual void sendResponse(NetworkResponse response, MemoryLevel level) = 0;

  // Make a load-linked reservation.
  virtual void makeReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) = 0;

  // Return whether a load-linked reservation is still valid.
  virtual bool checkReservation(ComponentID requester, MemoryAddr address, MemoryAccessMode mode) const = 0;

  // Check whether it is safe for the given operation to modify memory.
  virtual void preWriteCheck(const MemoryOperation& operation) const = 0;

  // Data access.
  // A "magic" access does not consume any bandwidth, and happens instantly.
  virtual uint32_t readWord(SRAMAddress position, MemoryAccessMode mode, bool magic=false) {
    checkAlignment(position, 4);

    if (!magic)
      bandwidth.recordEvent();

    return dataArray()[position/BYTES_PER_WORD];
  }

  virtual uint32_t readHalfword(SRAMAddress position, MemoryAccessMode mode, bool magic=false) {
    checkAlignment(position, 2);

    uint32_t fullWord = readWord(position & ~0x3, mode, magic);
    uint32_t offset = (position & 0x3) >> 1;
    return (fullWord >> (offset * 16)) & 0xFFFF;
  }

  virtual uint32_t readByte(SRAMAddress position, MemoryAccessMode mode, bool magic=false) {
    uint32_t fullWord = readWord(position & ~0x3, mode, magic);
    uint32_t offset = position & 0x3UL;
    return (fullWord >> (offset * 8)) & 0xFF;
  }

  virtual void writeWord(SRAMAddress position, uint32_t data, MemoryAccessMode mode, bool magic=false) {
    checkAlignment(position, 4);

    if (!magic)
      bandwidth.recordEvent();

    dataArray()[position/BYTES_PER_WORD] = data;
  }

  virtual void writeHalfword(SRAMAddress position, uint32_t data, MemoryAccessMode mode, bool magic=false) {
    checkAlignment(position, 2);

    uint32_t oldData = readWord(position & ~0x3, mode, true);
    uint32_t offset = (position >> 1) & 0x1;
    uint32_t mask = 0xFFFF << (offset * 16);
    uint32_t newData = (~mask & oldData) | (mask & (data << (16*offset)));

    writeWord(position & ~0x3, newData, mode, magic);
  }

  virtual void writeByte(SRAMAddress position, uint32_t data, MemoryAccessMode mode, bool magic=false) {
    uint32_t oldData = readWord(position & ~0x3, mode, true);
    uint32_t offset = position & 0x3;
    uint32_t mask = 0xFF << (offset * 8);
    uint32_t newData = (~mask & oldData) | (mask & (data << (8*offset)));

    writeWord(position & ~0x3, newData, mode, magic);
  }

  bool canRead() const {
    return bandwidth.bandwidthAvailable();
  }

  bool canWrite() const {
    return bandwidth.bandwidthAvailable();
  }

  // Memory address manipulation. Assumes fixed cache line size of 32 bytes.
  MemoryAddr getTag(MemoryAddr address) const {
    return address & ~offsetMask();
  }
  MemoryAddr getLine(SRAMAddress position) const {
    return position >> log2CacheLineSize;
  }
  MemoryAddr getOffset(SRAMAddress position) const {
    return position & offsetMask();
  }
  MemoryAddr offsetMask() const {
    return (1 << log2CacheLineSize) - 1;
  }


  // This would probably go better in some other class.
  static bool isPayload(NetworkRequest request) {
    MemoryOpcode opcode = request.getMemoryMetadata().opcode;
    return (opcode == PAYLOAD) || (opcode == PAYLOAD_EOP);
  }

  void checkAlignment(SRAMAddress position, uint alignment) const {
    MemoryAddr address = getAddress(position);
    if (WARN_UNALIGNED && (address & (alignment-1)) != 0)
      LOKI_WARN << " attempting to access address " << LOKI_HEX(address)
          << " with alignment " << alignment << "." << std::endl;
  }

  void printOperation(
      MemoryOpcode    operation,
      MemoryAddr      address,
      uint32_t        data
  ) const {
    LOKI_LOG(1) << this->name() << ": " << memoryOpName(operation) <<
        ", address = " << LOKI_HEX(address) << ", data = " << LOKI_HEX(data);

    if (operation == IPK_READ) {
      Instruction inst(data);
      LOKI_LOG(1) << " (" << inst << ")";
    }

    LOKI_LOG(1) << endl;
  }

  size_t cacheLineBytes() const {
    return 1 << log2CacheLineSize;
  }

  size_t cacheLineWords() const {
    return cacheLineBytes() / BYTES_PER_WORD;
  }

public:

  const ComponentID id;

protected:

  // Abstract away where the data is stored. Allows multiple memories to share
  // data.
  virtual const vector<uint32_t>& dataArray() const = 0;
  virtual vector<uint32_t>& dataArray() = 0;

  const size_t log2CacheLineSize; // In bytes

private:

  // Reads/writes possible in one cycle. A subword write is treated as a single
  // operation.
  BandwidthMonitor bandwidth;

};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBASE_H_ */
