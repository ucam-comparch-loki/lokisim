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
	// | Opcode : 8 | Way bits : 4 | Line bits : 4 | Mode : 8 | Group bits : 8 |
	// | Address : 32                                                          |
  // | Unused : 22                       | TileX : 3 | TileY : 3 | Entry : 4 |

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

	static const uint OFFSET_TILE             = 4;
	static const uint  WIDTH_TILE             = 6;
	static const uint OFFSET_DIRECTORY_ENTRY  = 0;
	static const uint  WIDTH_DIRECTORY_ENTRY  = 4;

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
		CONTROL               = 30, // Configuration - may not be needed

		NONE                  = 31
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

	inline MemoryOpCode    getOpCode()     const {return (MemoryOpCode)getBits(OFFSET_OPCODE, OFFSET_OPCODE + WIDTH_OPCODE - 1);}
	inline uint            getWayBits()    const {return (uint)getBits(OFFSET_WAY_BITS, OFFSET_WAY_BITS + WIDTH_WAY_BITS - 1);}
	inline uint            getLineBits()   const {return (uint)getBits(OFFSET_LINE_BITS, OFFSET_LINE_BITS + WIDTH_LINE_BITS - 1);}
	inline MemoryMode      getMode()       const {return (MemoryMode)getBits(OFFSET_MODE, OFFSET_MODE + WIDTH_MODE - 1);}
	inline uint            getGroupBits()  const {return getBits(OFFSET_GROUP_BITS, OFFSET_GROUP_BITS + WIDTH_GROUP_BITS - 1);}

	inline uint            getDirectoryEntry() const {return getBits(OFFSET_DIRECTORY_ENTRY, OFFSET_DIRECTORY_ENTRY + WIDTH_DIRECTORY_ENTRY - 1);}
	inline TileIndex       getTile()       const {return getBits(OFFSET_TILE, OFFSET_TILE + WIDTH_TILE - 1);}

	// Returns whether a single value is being retrieved.
	static inline bool isSingleLoad(MemoryOperation op) {
    switch (op) {
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
	static inline bool isSingleStore(MemoryOperation op) {
    switch (op) {
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
	static inline bool isReadModifyWrite(MemoryOperation op) {
	  switch (op) {
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

  static const string& name(MemoryOperation op) {
    static const string operationNames[] = {
      "LOAD_W",          "LOAD_HW",           "LOAD_B",                 "STORE_W",
      "STORE_HW",        "STORE_B",           "PAYLOAD",                "IPK_READ",
      "FETCH_LINE",      "STORE_LINE",        "FLUSH_LINE",             "MEMSET_LINE",
      "INVALIDATE_LINE", "VALIDATE_LINE",     "UPDATE_DIRECTORY_ENTRY", "UPDATE_DIRECTORY_MASK",
      "LOAD_LINKED",     "STORE_CONDITIONAL", "LOAD_AND_ADD",           "LOAD_AND_OR",
      "LOAD_AND_AND",    "LOAD_AND_XOR",      "EXCHANGE",               "INVALID1",
      "INVALID2",        "INVALID3",          "INVALID4",               "INVALID5",
      "INVALID6",        "INVALID7",          "CONTROL",                "NONE"
    };
	  return operationNames[(int)op];
	}

	MemoryRequest() : Word() {
		// Nothing
	}

	MemoryRequest(const Word& other) : Word(other) {
		// Nothing
	}

};

#endif /* MEMORYREQUEST_H_ */
