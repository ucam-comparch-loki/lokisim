/*
 * MemoryRequest.h
 *
 * Request to set up a channel with a memory. Specify the type of operation
 * to be performed and the address in memory to perform it.
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#ifndef MEMORYREQUEST_H_
#define MEMORYREQUEST_H_

#include "Word.h"
#include "Identifier.h"
#include "../Exceptions/InvalidOptionException.h"
#include "../Typedefs.h"
#include "../TileComponents/Memory/MemoryTypedefs.h"

class MemoryRequest : public Word {
private:
	// | Unused : 12    | Memory operation : 8 | Opcode : 8 | Way bits : 4 | Line bits : 4 | Mode : 8 | Group bits : 8 |
	// | Src tile:6 | Src bank:4 | Line size:2 | Memory operation : 8 | Address : 32                                                          |
	// | Unused : 12    | Memory operation : 8 | Burst length : 32                                                     |
	// | Unused : 12    | Memory operation : 8 | Channel ID : 32                                                       |
  // | Unused : 12    | Memory operation : 8 | Directory entry : 16 | Tile : 16                                      |

  static const uint OFFSET_SOURCE_TILE      = 46;
  static const uint  WIDTH_SOURCE_TILE      = 6;
  static const uint OFFSET_SOURCE_BANK      = 42;
  static const uint  WIDTH_SOURCE_BANK      = 4;
	static const uint OFFSET_LINE_SIZE        = 40;
	static const uint  WIDTH_LINE_SIZE        = 2;
	static const uint OFFSET_OPERATION        = 32;
	static const uint  WIDTH_OPERATION        = 8;

	static const uint OFFSET_OPCODE           = 24;
	static const uint  WIDTH_OPCODE           = 8;
	static const uint OFFSET_WAY_BITS         = 20;
	static const uint  WIDTH_WAY_BITS         = 4;
	static const uint OFFSET_LINE_BITS        = 16;
	static const uint  WIDTH_LINE_BITS        = 4;
	static const uint OFFSET_MODE             = 8;
	static const uint  WIDTH_MODE             = 8;
	static const uint OFFSET_GROUP_BITS       = 0;
	static const uint  WIDTH_GROUP_BITS       = 8;

	static const uint OFFSET_TILE             = 0;
	static const uint  WIDTH_TILE             = 16;
	static const uint OFFSET_DIRECTORY_ENTRY  = 16;
	static const uint  WIDTH_DIRECTORY_ENTRY  = 16;

public:
	enum MemoryOperation {
	  // Data access
		LOAD_W,
		LOAD_HW,
		LOAD_B,
		STORE_W,
		STORE_HW,
		STORE_B,
		PAYLOAD_ONLY,
		IPK_READ,

		// Cache control
		FETCH_LINE,
		STORE_LINE,
		FLUSH_LINE,
		MEMSET_LINE,
		INVALIDATE_LINE,
		VALIDATE_LINE,
		UPDATE_DIRECTORY_ENTRY,
		UPDATE_DIRECTORY_MASK,

		// Atomics
		LOAD_LINKED,
		STORE_CONDITIONAL,
		LOAD_AND_ADD,
		LOAD_AND_OR,
		LOAD_AND_AND,
		LOAD_AND_XOR,
		EXCHANGE,

		// Other
		CONTROL               = 31, // Configuration - may not be needed

		NONE                  = 255
	};

	enum MemoryOpCode {
		SET_MODE              = 0,  // Update mode (cache/scratchpad - see below)
		SET_CHMAP             = 1   // Update channel map table
	};

	enum MemoryMode {
		SCRATCHPAD            = 0,  // Scratchpad mode
		GP_CACHE              = 1   // General-purpose cache mode
	};

	inline uint32_t        getPayload()    const {return data_ & 0xFFFFFFFFULL;}
	inline ChannelID       getChannelID()  const {return ChannelID(data_ & 0xFFFFFFFFULL);}

	// Line size in words.
	inline uint            getLineSize()   const {
	  uint encodedLineSize = getBits(OFFSET_LINE_SIZE, OFFSET_LINE_SIZE + WIDTH_LINE_SIZE - 1);
	  switch (encodedLineSize) {
	    case LS_4:  return 4;
	    case LS_8:  return 8;
	    case LS_16: return 16;
	    case LS_32: return 32;
	    default:
	      throw InvalidOptionException("encoded line size", encodedLineSize);
	      break;
	  }
	}

	inline TileID          getSourceTile() const {return TileID(getBits(OFFSET_SOURCE_TILE, OFFSET_SOURCE_TILE + WIDTH_SOURCE_TILE - 1));}
	inline MemoryIndex     getSourceBank() const {return getBits(OFFSET_SOURCE_BANK, OFFSET_SOURCE_BANK + WIDTH_SOURCE_BANK - 1);}
	inline MemoryOperation getOperation()  const {return (MemoryOperation)getBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1);}
	inline MemoryOpCode    getOpCode()     const {return (MemoryOpCode)getBits(OFFSET_OPCODE, OFFSET_OPCODE + WIDTH_OPCODE - 1);}
	inline uint            getWayBits()    const {return (uint)getBits(OFFSET_WAY_BITS, OFFSET_WAY_BITS + WIDTH_WAY_BITS - 1);}
	inline uint            getLineBits()   const {return (uint)getBits(OFFSET_LINE_BITS, OFFSET_LINE_BITS + WIDTH_LINE_BITS - 1);}
	inline MemoryMode      getMode()       const {return (MemoryMode)getBits(OFFSET_MODE, OFFSET_MODE + WIDTH_MODE - 1);}
	inline uint            getGroupBits()  const {return getBits(OFFSET_GROUP_BITS, OFFSET_GROUP_BITS + WIDTH_GROUP_BITS - 1);}

	inline uint            getDirectoryEntry() const {return getBits(OFFSET_DIRECTORY_ENTRY, OFFSET_DIRECTORY_ENTRY + WIDTH_DIRECTORY_ENTRY - 1);}
	inline TileIndex       getTile()       const {return getBits(OFFSET_TILE, OFFSET_TILE + WIDTH_TILE - 1);}

	// Returns whether the next level of cache should also be accessed.
	inline bool isThroughAccess() const {
	  return false;
	}

	// Once the request has been sent to the next cache level, this becomes a
	// normal memory operation at this level.
	inline void clearThroughAccess() {
	  assert(isThroughAccess());

	  // TODO: is this method unnecessary?
	}

	// Returns whether a single value is being retrieved.
	inline bool isSingleLoad() const {
    switch (getOperation()) {
      case LOAD_W:
      case LOAD_HW:
      case LOAD_B:
      case LOAD_LINKED:
        return true;
      default:
        return false;
    }
	}

	// Returns whether a single value is being stored.
	inline bool isSingleStore() const {
    switch (getOperation()) {
      case STORE_W:
      case STORE_HW:
      case STORE_B:
      case STORE_CONDITIONAL:
        return true;
      default:
        return false;
    }
	}

	// Returns whether this operation both reads and writes a value.
	inline bool isReadModifyWrite() const {
	  switch (getOperation()) {
      case LOAD_AND_ADD:
      case LOAD_AND_OR:
      case LOAD_AND_AND:
      case LOAD_AND_XOR:
      case EXCHANGE:
        return true;
      default:
        return false;
	  }
	}

	MemoryRequest() : Word() {
		// Nothing
	}

	// Line size should be in words.
	MemoryRequest(MemoryOperation operation, uint32_t payload, uint lineSize, TileID sourceTile, MemoryIndex sourceBank) : Word() {
		// Encode lineSize to reduce bits required.
	  LineSizeWords encodedLineSize;
	  switch (lineSize) {
	    case 4:   encodedLineSize = LS_4; break;
	    case 8:   encodedLineSize = LS_8; break;
	    case 16:  encodedLineSize = LS_16; break;
	    case 32:  encodedLineSize = LS_32; break;
	    default: throw InvalidOptionException("line size", lineSize);
	  }

	  data_ = (((int64_t)operation) << OFFSET_OPERATION) | (((int64_t)encodedLineSize) << OFFSET_LINE_SIZE) | (((int64_t)sourceTile.flatten()) << OFFSET_SOURCE_TILE) | (((int64_t)sourceBank) << OFFSET_SOURCE_BANK) | payload;
	}

	MemoryRequest(MemoryOperation operation, uint32_t payload) : Word() {
		data_ = (((int64_t)operation) << OFFSET_OPERATION) | payload;
	}

	MemoryRequest(const Word& other) : Word(other) {
		// Nothing
	}

private:

	void setOperation(MemoryOperation op) {
	  setBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1, op);
	}

};

#endif /* MEMORYREQUEST_H_ */
