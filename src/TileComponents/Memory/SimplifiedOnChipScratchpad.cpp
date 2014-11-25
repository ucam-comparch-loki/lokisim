//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Simplified On-Chip Scratchpad Implementation
//-------------------------------------------------------------------------------------------------
// Second version of simplified background memory model.
//
// Models a simplified on-chip scratchpad with an unlimited number of ports.
// The scratchpad is connected through an abstract on-chip network and
// serves as the background memory for the cache mode(s) of the memory banks.
//
// The number of input and output ports is configurable. The ports possess
// queues for incoming and outgoing data.
//
// Colliding write requests are executed in ascending port order.
//
// Communication protocol:
//
// 1. Send command word - highest bit: 0 = read, 1 = write, lower bits word count
// 2. Send start address
// 3. Send words in case of write command
//-------------------------------------------------------------------------------------------------
// File:       SimplifiedOnChipScratchpad.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 08/04/2011
//-------------------------------------------------------------------------------------------------

#include <iostream>
#include <iomanip>

using namespace std;

#include "../../Component.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Datatype/Packets/CacheLineFlush.h"
#include "../../Datatype/Packets/CacheLineReadRequest.h"
#include "../../Datatype/Packets/CacheLineRefill.h"
#include "../../Utility/Instrumentation.h"
#include "SimplifiedOnChipScratchpad.h"

void SimplifiedOnChipScratchpad::tryStartRequest(uint port) {
	mPortData[port].State = STATE_IDLE;

	assert(mInputPacket == NULL);
	assert(mOutputPacket == NULL);

	if (!mInputQueues[port].empty() && mCycleCounter >= mInputQueues[port].peek().EarliestExecutionCycle) {
		const NetworkRequest& flit = mInputQueues[port].read().Request;
	  MemoryRequest request = flit.payload();

		if (request.getOperation() == MemoryRequest::FETCH_LINE) {
		  mInputPacket = new CacheLineReadRequest();
		  mInputPacket->addFlit(flit);

			mPortData[port].MemoryAddress = request.getPayload();
			mPortData[port].WordsLeft = request.getLineSize() / 4;

			if (mPortData[port].MemoryAddress + mPortData[port].WordsLeft * 4 > MEMORY_ON_CHIP_SCRATCHPAD_SIZE)
				cerr << this->name() << " fetch request outside valid memory space (address " << mPortData[port].MemoryAddress << ", length " << (mPortData[port].WordsLeft * 4) << ")" << endl;

			assert((mPortData[port].MemoryAddress & 0x3) == 0);
			assert(mPortData[port].MemoryAddress <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert((mPortData[port].WordsLeft * 4) <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert(mPortData[port].MemoryAddress + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert(mPortData[port].WordsLeft > 0);

			Instrumentation::backgroundMemoryRead(mPortData[port].MemoryAddress, mPortData[port].WordsLeft);

			// Do not output first word directly - assume one clock cycle access delay

			mPortData[port].State = STATE_READING;
		} else if (request.getOperation() == MemoryRequest::STORE_LINE) {
		  mInputPacket = new CacheLineFlush();
		  mInputPacket->addFlit(flit);

			mPortData[port].MemoryAddress = request.getPayload();
			mPortData[port].WordsLeft = request.getLineSize() / 4;

			if (mPortData[port].MemoryAddress + mPortData[port].WordsLeft * 4 > MEMORY_ON_CHIP_SCRATCHPAD_SIZE)
				cerr << this->name() << " store request outside valid memory space (address " << mPortData[port].MemoryAddress << ", length " << (mPortData[port].WordsLeft * 4) << ")" << endl;

			assert((mPortData[port].MemoryAddress & 0x3) == 0);
			assert(mPortData[port].MemoryAddress + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert(mPortData[port].WordsLeft > 0);

			Instrumentation::backgroundMemoryWrite(mPortData[port].MemoryAddress, mPortData[port].WordsLeft);

			mPortData[port].State = STATE_WRITING;
		} else {
		  throw InvalidOptionException("memory operation", request.getOperation());
		}
	}
}

void SimplifiedOnChipScratchpad::mainLoop() {
	assert(cBanks <= 16);

	for (;;) {
		// Wait for start of clock cycle

		wait(iClock.posedge_event());

		// Handle input data, arbitrating fairly between requests and responses.
		// Once we start receiving a packet from one input, continue until the
		// packet is finished, and then give the other input priority.

		if (!mInputQueues[0].full()) {
      if (iRequest.valid() && ((priorityInput == INPUT_REQUEST) ||
                               (finishedPacket && !iResponse.valid()))) {
        InputWord newWord;
        newWord.EarliestExecutionCycle = mCycleCounter + cDelayCycles;
        newWord.Request = iRequest.read();
        iRequest.ack();
        mInputQueues[0].write(newWord);

        finishedPacket = newWord.Request.endOfPacket();
        if (finishedPacket)
          priorityInput = INPUT_RESPONSE;
      }
      else if (iResponse.valid() && ((priorityInput == INPUT_RESPONSE) ||
                                     (finishedPacket && !iRequest.valid()))) {
        InputWord newWord;
        newWord.EarliestExecutionCycle = mCycleCounter + cDelayCycles;
        newWord.Request = iResponse.read();
        iResponse.ack();
        mInputQueues[0].write(newWord);

        finishedPacket = newWord.Request.endOfPacket();
        if (finishedPacket)
          priorityInput = INPUT_REQUEST;
      }
		}

		// Process port events

    switch (mPortData[0].State) {
    case STATE_IDLE:
      tryStartRequest(0);
      break;

    case STATE_READING:
      assert(mInputPacket != NULL);

      if (!mInputPacket->receivedAllFlits() && !mInputQueues[0].empty())
        mInputPacket->addFlit(mInputQueues[0].read().Request);
      if (mInputPacket->receivedAllFlits() && mOutputPacket == NULL) {
        CacheLineReadRequest* request = static_cast<CacheLineReadRequest*>(mInputPacket);
        mOutputPacket = new CacheLineRefill(&(mData[request->memoryAddress() / 4]),
                                            request->lineSize(),
                                            request->returnAddress());
      }
      if (mOutputPacket != NULL && !oResponse.valid()) {
        oResponse.write(mOutputPacket->getNextFlit());
        mOutputPacket->sentFlit();

        if (mOutputPacket->sentAllFlits()) {
          delete mInputPacket;
          mInputPacket = NULL;
          delete mOutputPacket;
          mOutputPacket = NULL;

          tryStartRequest(0);
        }
      }

      break;

    case STATE_WRITING:
      assert(mInputPacket != NULL);

      if (!mInputPacket->receivedAllFlits() && !mInputQueues[0].empty()) {
        CacheLineFlush* request = static_cast<CacheLineFlush*>(mInputPacket);
        request->addFlit(mInputQueues[0].read().Request);
        // Extract data and write it
        uint words = request->numFlits() - 1;        // -1 because of the header
        uint data = request->data(words-1).toUInt(); // -1 because counting from 0
        MemoryAddr address = (request->memoryAddress() / 4) + (words-1);
        mData[address] = data;

        if (mInputPacket->receivedAllFlits()) {
          delete mInputPacket;
          mInputPacket = NULL;

          tryStartRequest(0);
        }
      }

      break;
    } // end switch

		// Advance cycle counter

		mCycleCounter++;
	}
}

SimplifiedOnChipScratchpad::SimplifiedOnChipScratchpad(sc_module_name name, const ComponentID& ID, uint portCount) :
	Component(name, ID),
	mInputQueues(portCount, 1024, "SimplifiedOnChipScratchpad::mInputQueues")	// Virtually infinite queue length
{
	assert(portCount >= 1);

	cDelayCycles = MEMORY_ON_CHIP_SCRATCHPAD_DELAY;
	cBanks = MEMORY_ON_CHIP_SCRATCHPAD_BANKS;

	oReadyRequest.initialize(true);
	oReadyResponse.initialize(true);

	priorityInput = INPUT_REQUEST;

	mCycleCounter = 0;

	mData = new uint32_t[MEMORY_ON_CHIP_SCRATCHPAD_SIZE / 4];
	memset(mData, 0x00, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

	mInputPacket = NULL;
	mOutputPacket = NULL;
	finishedPacket = true;

	mPortCount = portCount;
	mPortData = new PortData[portCount];

	for (uint i = 0; i < portCount; i++)
		mPortData[i].State = STATE_IDLE;

	// In most cases, we are now trying to replace threads with methods, but this
	// one seems to perform better as a thread.
	SC_THREAD(mainLoop);
}

SimplifiedOnChipScratchpad::~SimplifiedOnChipScratchpad() {
	delete[] mData;
	delete[] mPortData;
}

/*
void SimplifiedOnChipScratchpad::flushQueues() {
	// Process port events

	bool dataProcessed;

	do {
		dataProcessed = false;

		for (uint port = 0; port < mPortCount; port++) {
			assert(mPortData[port].State == STATE_IDLE || mPortData[port].State == STATE_WRITING);

			if (mPortData[port].State == STATE_IDLE) {
				if (!mInputQueues[port].empty()) {
					MemoryRequest request = mInputQueues[port].read().Request;

					assert(request.getOperation() == MemoryRequest::STORE_LINE);

					mPortData[port].MemoryAddress = request.getPayload();
					mPortData[port].WordsLeft = request.getLineSize() / 4;

					if (mPortData[port].MemoryAddress + mPortData[port].WordsLeft * 4 > MEMORY_ON_CHIP_SCRATCHPAD_SIZE)
						cerr << this->name() << " store request outside valid memory space (address " << mPortData[port].MemoryAddress << ", length " << (mPortData[port].WordsLeft * 4) << ")" << endl;

					assert((mPortData[port].MemoryAddress & 0x3) == 0);
					assert(mPortData[port].MemoryAddress + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
					assert(mPortData[port].WordsLeft > 0);

					Instrumentation::backgroundMemoryWrite(mPortData[port].MemoryAddress, mPortData[port].WordsLeft);

					mPortData[port].State = STATE_WRITING;

					dataProcessed = true;
				}
			} else {
				assert(!mInputQueues[port].empty());
				assert(mInputQueues[port].peek().Request.getOperation() == MemoryRequest::PAYLOAD_ONLY);

				mData[mPortData[port].MemoryAddress / 4] = mInputQueues[port].read().Request.getPayload();
				mPortData[port].MemoryAddress += 4;
				mPortData[port].WordsLeft--;

				if (mPortData[port].WordsLeft == 0)
					mPortData[port].State = STATE_IDLE;

				dataProcessed = true;
			}
		}
	} while (dataProcessed);
}
*/

ChannelID SimplifiedOnChipScratchpad::networkAddress() const {
  // Give the memory an address beyond that of any of the compute tiles.
  // It will be positioned in the first column, at the bottom of the chip.
  return ChannelID(NUM_TILES, 0, 0);
}

void SimplifiedOnChipScratchpad::storeData(vector<Word>& data, MemoryAddr location, bool readOnly) {
	size_t count = data.size();
	uint32_t address = location / 4;

	assert(location % 4 == 0);
	assert(location + count * 4 < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

	for (size_t i = 0; i < count; i++) {
		if (DEBUG)
			cout << this->name() << " store 0x" << setprecision(8) << setfill('0') << hex << ((address + i) * 4) << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data[i].toUInt() << endl << dec;

		mData[address + i] = data[i].toUInt();
	}

	if (readOnly) {
	  readOnlyBase.push_back(location);
	  readOnlyLimit.push_back(location + 4*data.size());
	}
}

const void* SimplifiedOnChipScratchpad::getData() {
	return mData;
}

bool SimplifiedOnChipScratchpad::readOnly(MemoryAddr addr) const {
  for (uint i=0; i<readOnlyBase.size(); i++) {
    if ((addr >= readOnlyBase[i]) && (addr < readOnlyLimit[i])) {
      return true;
    }
  }

  return false;
}

void SimplifiedOnChipScratchpad::print(MemoryAddr start, MemoryAddr end) {
	if (start > end) {
		MemoryAddr temp = start;
		start = end;
		end = temp;
	}

	size_t address = start / 4;
	size_t limit = end / 4 + 1;

	cout << "On-chip scratchpad data:\n" << endl;

	while (address < limit) {
		assert(address * 4 < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
		cout << "0x" << setprecision(8) << setfill('0') << hex << (address * 4) << ":  " << "0x" << setprecision(8) << setfill('0') << hex << mData[address] << endl << dec;
		address++;
	}
}

Word SimplifiedOnChipScratchpad::readWord(MemoryAddr addr) {
	assert(addr < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	assert(addr % 4 == 0);
	return Word(mData[addr / 4]);
}

Word SimplifiedOnChipScratchpad::readByte(MemoryAddr addr) {
	assert(addr < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	uint32_t data = mData[addr / 4];
	uint shift = (addr & 0x3UL) * 8;
	return Word((data >> shift) & 0xFFUL);
}

void SimplifiedOnChipScratchpad::writeWord(MemoryAddr addr, Word data) {
	assert(addr < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	assert(addr % 4 == 0);
	assert(!readOnly(addr));
	mData[addr / 4] = data.toUInt();
}

void SimplifiedOnChipScratchpad::writeByte(MemoryAddr addr, Word data) {
	assert(addr < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	assert(!readOnly(addr));
	uint32_t modData = mData[addr / 4];
	uint shift = (addr & 0x3UL) * 8;
	uint32_t mask = 0xFFUL << shift;
	modData &= ~mask;
	modData |= (data.toUInt() & 0xFFUL) << shift;
	mData[addr / 4] = modData;
}
