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
#include "ChannelID.h"
#include "../Typedefs.h"
#include "../Exceptions/InvalidOptionException.h"

class MemoryRequest : public Word {
private:
	// | Unused : 12    | Memory operation : 8 | Opcode : 8 | Way bits : 4 | Line bits : 4 | Mode : 8 | Group bits : 8 |
	// | Line size : 12 | Memory operation : 8 | Address : 32                                                          |
	// | Unused : 12    | Memory operation : 8 | Burst length : 32                                                     |
	// | Unused : 12    | Memory operation : 8 | Channel ID : 32                                                       |

	static const uint OFFSET_LINE_SIZE = 40;
	static const uint WIDTH_LINE_SIZE = 12;
	static const uint OFFSET_OPERATION = 32;
	static const uint WIDTH_OPERATION = 8;

	static const uint OFFSET_OPCODE = 24;
	static const uint WIDTH_OPCODE = 8;
	static const uint OFFSET_WAY_BITS = 20;
	static const uint WIDTH_WAY_BITS = 4;
	static const uint OFFSET_LINE_BITS = 16;
	static const uint WIDTH_LINE_BITS = 4;
	static const uint OFFSET_MODE = 8;
	static const uint WIDTH_MODE = 8;
	static const uint OFFSET_GROUP_BITS = 0;
	static const uint WIDTH_GROUP_BITS = 8;
public:
	enum MemoryOperation {
		CONTROL = 0,
		PAYLOAD_ONLY = 1,
		LOAD_W = 2,
		LOAD_HW = 3,
		LOAD_B = 4,
		STORE_W = 5,
		STORE_HW = 6,
		STORE_B = 7,
		IPK_READ = 8,
		BURST_READ = 9,
		BURST_WRITE = 10,
		FETCH_LINE = 11,
		STORE_LINE = 12,

		LOAD_THROUGH_W = 18,    // Go straight to next level of cache
		LOAD_THROUGH_HW = 19,   // Go straight to next level of cache
		LOAD_THROUGH_B = 20,    // Go straight to next level of cache
		STORE_THROUGH_W = 21,   // Store to this cache and the one backing it
		STORE_THROUGH_HW = 22,  // Store to this cache and the one backing it
		STORE_THROUGH_B = 23,   // Store to this cache and the one backing it

		NONE = 255
	};

	enum MemoryOpCode {
		SET_MODE = 0,
		SET_CHMAP = 1
	};

	enum MemoryMode {
		SCRATCHPAD = 0,
		GP_CACHE = 1
	};

	inline uint32_t getPayload() const						{return data_ & 0xFFFFFFFFULL;}
	inline ChannelID getChannelID() const					{return ChannelID(data_ & 0xFFFFFFFFULL);}

	inline uint getLineSize() const							{return (uint)getBits(OFFSET_LINE_SIZE, OFFSET_LINE_SIZE + WIDTH_LINE_SIZE - 1);}
	inline MemoryOperation getOperation() const				{return (MemoryOperation)getBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1);}
	inline MemoryOpCode getOpCode() const					{return (MemoryOpCode)getBits(OFFSET_OPCODE, OFFSET_OPCODE + WIDTH_OPCODE - 1);}
	inline uint getWayBits() const							{return (uint)getBits(OFFSET_WAY_BITS, OFFSET_WAY_BITS + WIDTH_WAY_BITS - 1);}
	inline uint getLineBits() const							{return (uint)getBits(OFFSET_LINE_BITS, OFFSET_LINE_BITS + WIDTH_LINE_BITS - 1);}
	inline MemoryMode getMode() const						{return (MemoryMode)getBits(OFFSET_MODE, OFFSET_MODE + WIDTH_MODE - 1);}
	inline uint getGroupBits() const						{return getBits(OFFSET_GROUP_BITS, OFFSET_GROUP_BITS + WIDTH_GROUP_BITS - 1);}

	// Returns whether the next level of cache should also be accessed.
	inline bool isThroughAccess() const {
	  switch (getOperation()) {
      case LOAD_THROUGH_W:
      case LOAD_THROUGH_HW:
      case LOAD_THROUGH_B:
      case STORE_THROUGH_W:
      case STORE_THROUGH_HW:
      case STORE_THROUGH_B:
	      return true;
	    default:
	      return false;
	  }
	}

	// Once the request has been sent to the next cache level, this becomes a
	// normal memory operation at this level.
	inline void clearThroughAccess() {
	  assert(isThroughAccess());

	  switch (getOperation()) {
      case LOAD_THROUGH_W:        setOperation(LOAD_W);   break;
      case LOAD_THROUGH_HW:       setOperation(LOAD_HW);  break;
      case LOAD_THROUGH_B:        setOperation(LOAD_B);   break;
      case STORE_THROUGH_W:       setOperation(STORE_W);  break;
      case STORE_THROUGH_HW:      setOperation(STORE_HW); break;
      case STORE_THROUGH_B:       setOperation(STORE_B);  break;
      default:
        throw InvalidOptionException("memory operation", getOperation());
        break;
	  }

	}

	// Returns whether a single value is being retrieved.
	inline bool isSingleLoad() const {
    switch (getOperation()) {
      case LOAD_W:
      case LOAD_HW:
      case LOAD_B:
      case LOAD_THROUGH_W:
      case LOAD_THROUGH_HW:
      case LOAD_THROUGH_B:
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
      case STORE_THROUGH_W:
      case STORE_THROUGH_HW:
      case STORE_THROUGH_B:
        return true;
      default:
        return false;
    }
	}



	MemoryRequest() : Word() {
		// Nothing
	}

	MemoryRequest(MemoryOperation operation, uint32_t payload, uint lineSize) : Word() {
		data_ = (((int64_t)operation) << OFFSET_OPERATION) | (((int64_t)lineSize) << OFFSET_LINE_SIZE) | payload;
	}

	MemoryRequest(MemoryOperation operation, uint32_t payload) : Word() {
		data_ = (((int64_t)operation) << OFFSET_OPERATION) | payload;
	}

	MemoryRequest(MemoryOperation operation, const ChannelID& channel) : Word() {
		data_ = (((int64_t)operation) << OFFSET_OPERATION) | channel.getData();
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
