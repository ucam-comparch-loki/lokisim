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
	static const uint OFFSET_OPERATION = 0;
	static const uint WIDTH_OPERATION = 4;

	static const uint OFFSET_MODE = 4;
	static const uint WIDTH_MODE = 4;
	static const uint OFFSET_GROUP_SIZE = 16;
	static const uint WIDTH_GROUP_SIZE = 8;

	static const uint OFFSET_CHANNEL_ID = 32;		// Simplification for modelling purposes
	static const uint WIDTH_CHANNEL_ID = 32;

	static const uint OFFSET_ADDRESS_LONG = 8;
	static const uint WIDTH_ADDRESS_LONG = 24;

	static const uint OFFSET_BURST_LENGTH = 8;
	static const uint WIDTH_BURST_LENGTH = 8;
	static const uint OFFSET_ADDRESS_SHORT = 16;
	static const uint WIDTH_ADDRESS_SHORT = 16;
public:
	enum MemoryOperation {
		NONE = 0,
		SET_MODE,
		SET_CHMAP,
		LOAD_W,
		LOAD_HW,
		LOAD_B,
		STORE_W,
		STORE_HW,
		STORE_B,
		IPK_READ,
		BURST_READ,
		BURST_WRITE
	};

	enum MemoryMode {
		SCRATCHPAD = 0,
		GP_CACHE
	};

	static const uint32_t ESCAPE_ADDRESS_LONG = 0xFFFFFFUL;
	static const uint32_t ESCAPE_ADDRESS_SHORT = 0xFFFFUL;

	inline uint32_t getData() const							{return data_ & 0xFFFFFFFFULL;}
	inline ChannelID getChannelID() const					{return ChannelID(getBits(OFFSET_CHANNEL_ID, OFFSET_CHANNEL_ID + WIDTH_CHANNEL_ID - 1));}

	inline MemoryOperation getOperation() const				{return (MemoryOperation)getBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1);}
	inline MemoryMode getMode() const						{return (MemoryMode)getBits(OFFSET_MODE, OFFSET_MODE + WIDTH_MODE - 1);}
	inline uint getGroupSize() const						{return getBits(OFFSET_GROUP_SIZE, OFFSET_GROUP_SIZE + WIDTH_GROUP_SIZE - 1);}
	inline uint32_t getAddressLong() const					{return getBits(OFFSET_ADDRESS_LONG, OFFSET_ADDRESS_LONG + WIDTH_ADDRESS_LONG - 1);}
	inline uint32_t getAddressShort() const					{return getBits(OFFSET_ADDRESS_SHORT, OFFSET_ADDRESS_SHORT + WIDTH_ADDRESS_SHORT - 1);}
	inline uint getBurstLength() const						{return getBits(OFFSET_BURST_LENGTH, OFFSET_BURST_LENGTH + WIDTH_BURST_LENGTH - 1);}

	inline void setData(uint32_t data)						{data_ = data;}
	inline void setChannelID(const ChannelID& channelID)	{data_ = 0; setBits(OFFSET_CHANNEL_ID, OFFSET_CHANNEL_ID + WIDTH_CHANNEL_ID - 1, channelID.getData());}

	void setHeaderSetMode(MemoryMode mode, uint groupSize);
	void setHeaderSetTableEntry(const ChannelID& returnID);
	void setHeader(MemoryOperation operation, uint32_t address);
	void setHeader(MemoryOperation operation, uint burstLength, uint32_t address);

	MemoryRequest();
	MemoryRequest(uint32_t opaque);
	MemoryRequest(const Word& other);
};

#endif /* MEMORYREQUEST_H_ */
