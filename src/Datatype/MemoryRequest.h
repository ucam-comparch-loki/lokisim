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

class MemoryRequest : public Word {
private:
	// | Memory operation : 4 | Opcode : 8 | Unused : 8 | Mode : 8 | Group bits : 8 |
	// | Memory operation : 4 | Address : 32                                        |
	// | Memory operation : 4 | Burst length : 32                                   |
	// | Memory operation : 4 | Channel ID : 32                                     |

	static const uint OFFSET_OPERATION = 32;
	static const uint WIDTH_OPERATION = 4;

	static const uint OFFSET_OPCODE = 24;
	static const uint WIDTH_OPCODE = 8;
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
		NONE = 15
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

	inline MemoryOperation getOperation() const				{return (MemoryOperation)(data_ >> OFFSET_OPERATION);}
	inline MemoryOpCode getOpCode() const					{return (MemoryOpCode)getBits(OFFSET_OPCODE, OFFSET_OPCODE + WIDTH_OPCODE - 1);}
	inline MemoryMode getMode() const						{return (MemoryMode)getBits(OFFSET_MODE, OFFSET_MODE + WIDTH_MODE - 1);}
	inline uint getGroupBits() const						{return getBits(OFFSET_GROUP_BITS, OFFSET_GROUP_BITS + WIDTH_GROUP_BITS - 1);}


	MemoryRequest() : Word() {
		// Nothing
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
};

#endif /* MEMORYREQUEST_H_ */
