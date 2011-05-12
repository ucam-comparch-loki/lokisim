/*
 * MemoryRequest.cpp
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#include "MemoryRequest.h"

void MemoryRequest::setHeaderSetMode(MemoryMode mode, uint groupSize) {
	assert(groupSize < (1UL << WIDTH_GROUP_SIZE));

	data_ = 0;
	setBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1, SET_MODE);
	setBits(OFFSET_MODE, OFFSET_MODE + WIDTH_MODE - 1, mode);
	setBits(OFFSET_GROUP_SIZE, OFFSET_GROUP_SIZE + WIDTH_GROUP_SIZE - 1, groupSize);
}

void MemoryRequest::setHeaderSetTableEntry(const ChannelID& returnID) {
	data_ = 0;
	setBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1, SET_CHMAP);
	setBits(OFFSET_CHANNEL_ID, OFFSET_CHANNEL_ID + WIDTH_CHANNEL_ID - 1, returnID.getData());
}

void MemoryRequest::setHeader(MemoryOperation operation, uint32_t address) {
	assert(operation == LOAD_W || operation == LOAD_HW || operation == LOAD_B || operation == STORE_W || operation == STORE_HW || operation == STORE_B || operation == IPK_READ);
	assert(address <= ESCAPE_ADDRESS_LONG);

	data_ = 0;
	setBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1, operation);
	setBits(OFFSET_ADDRESS_LONG, OFFSET_ADDRESS_LONG + WIDTH_ADDRESS_LONG - 1, address);
}

void MemoryRequest::setHeader(MemoryOperation operation, uint burstLength, uint32_t address) {
	assert(operation == BURST_READ || operation == BURST_WRITE);
	assert(address <= ESCAPE_ADDRESS_SHORT);
	assert(burstLength < (1UL << WIDTH_BURST_LENGTH));

	data_ = 0;
	setBits(OFFSET_OPERATION, OFFSET_OPERATION + WIDTH_OPERATION - 1, operation);
	setBits(OFFSET_BURST_LENGTH, OFFSET_BURST_LENGTH + WIDTH_BURST_LENGTH - 1, burstLength);
	setBits(OFFSET_ADDRESS_SHORT, OFFSET_ADDRESS_SHORT + WIDTH_ADDRESS_SHORT - 1, address);
}

MemoryRequest::MemoryRequest() : Word() {
	// Nothing
}

MemoryRequest::MemoryRequest(uint32_t opaque) : Word(opaque) {
	// Nothing
}

MemoryRequest::MemoryRequest(const Word& other) : Word(other) {
	// Nothing
}
