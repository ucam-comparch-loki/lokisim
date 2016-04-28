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

#include "../../Component.h"
#include "MemoryTypedefs.h"
#include "../../Datatype/Instruction.h"
#include "../../Network/NetworkTypedefs.h"
#include "../../Utility/Warnings.h"

class MemoryBase : public Component {

public:

  MemoryBase(sc_module_name name, ComponentID ID, size_t sizeInBytes) :
    Component(name, ID),
    mData(sizeInBytes/4, 0) {

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
  virtual void makeReservation(ComponentID requester, MemoryAddr address) = 0;

  // Return whether a load-linked reservation is still valid.
  virtual bool checkReservation(ComponentID requester, MemoryAddr address) const = 0;

  // Check whether it is safe to write to the given address.
  virtual void preWriteCheck(MemoryAddr address) const = 0;

  // Data access.
  virtual uint32_t readWord(SRAMAddress position) const {
    checkAlignment(position, 4);

    return mData[position/BYTES_PER_WORD];
  }

  virtual uint32_t readHalfword(SRAMAddress position) const {
    checkAlignment(position, 2);

    uint32_t fullWord = readWord(position & ~0x3);
    uint32_t offset = (position & 0x3) >> 1;
    return (fullWord >> (offset * 16)) & 0xFFFF;
  }

  virtual uint32_t readByte(SRAMAddress position) const {
    uint32_t fullWord = readWord(position & ~0x3);
    uint32_t offset = position & 0x3UL;
    return (fullWord >> (offset * 8)) & 0xFF;
  }

  virtual void writeWord(SRAMAddress position, uint32_t data) {
    checkAlignment(position, 4);

    mData[position/BYTES_PER_WORD] = data;
  }

  virtual void writeHalfword(SRAMAddress position, uint32_t data) {
    checkAlignment(position, 2);

    uint32_t oldData = readWord(position & ~0x3);
    uint32_t offset = (position >> 1) & 0x1;
    uint32_t mask = 0xFFFF << (offset * 16);
    uint32_t newData = (~mask & oldData) | (mask & (data << (16*offset)));

    writeWord(position & ~0x3, newData);
  }

  virtual void writeByte(SRAMAddress position, uint32_t data) {
    uint32_t oldData = readWord(position & ~0x3);
    uint32_t offset = position & 0x3;
    uint32_t mask = 0xFF << (offset * 8);
    uint32_t newData = (~mask & oldData) | (mask & (data << (8*offset)));

    writeWord(position & ~0x3, newData);
  }

  // Memory address manipulation. Assumes fixed cache line size of 32 bytes.
  MemoryAddr getTag(MemoryAddr address)      const {return address & ~0x1F;}
  MemoryAddr getLine(SRAMAddress position)   const {return position >> 5;}
  MemoryAddr getOffset(SRAMAddress position) const {return position & 0x1F;}

  void printOperation(
      MemoryOpcode    operation,
      MemoryAddr      address,
      uint32_t        data
  ) const {
    LOKI_LOG << this->name() << ": " << memoryOpName(operation) <<
        ", address = " << LOKI_HEX(address) << ", data = " << LOKI_HEX(data);

    if (operation == IPK_READ) {
      Instruction inst(data);
      LOKI_LOG << " (" << inst << ")";
    }

    LOKI_LOG << endl;
  }

protected:

  void checkAlignment(SRAMAddress position, uint alignment) const {
    MemoryAddr address = getAddress(position);
    if (WARN_UNALIGNED && (address & (alignment-1)) != 0)
      LOKI_WARN << " attempting to access address " << LOKI_HEX(address)
          << " with alignment " << alignment << "." << std::endl;
  }

  // This would probably go better in some other class.
  bool isPayload(NetworkRequest request) const {
    MemoryOpcode opcode = request.getMemoryMetadata().opcode;
    return (opcode == PAYLOAD) || (opcode == PAYLOAD_EOP);
  }

protected:

  // The stored data.
  vector<uint32_t>   mData;

};

#endif /* SRC_TILECOMPONENTS_MEMORY_OPERATIONS_MEMORYBASE_H_ */
