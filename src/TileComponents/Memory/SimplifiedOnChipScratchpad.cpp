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

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <iostream>
#include <iomanip>

using namespace std;

#include "../../Component.h"
#include "../../Utility/Instrumentation.h"
#include "SimplifiedOnChipScratchpad.h"

void SimplifiedOnChipScratchpad::tryStartRequest(uint port) {
	mPortData[port].State = STATE_IDLE;

	if (!mInputQueues[port].empty() && mCycleCounter >= mInputQueues[port].peek().EarliestExecutionCycle) {
		NetworkRequest request = mInputQueues[port].peek().Request;

		if (request.getMemoryMetadata().opcode == FETCH_LINE) {
			mPortData[port].Address = request.payload().toUInt();
			mPortData[port].WordsLeft = CACHE_LINE_WORDS;
			mPortData[port].ReturnAddress = ChannelID(request.getMemoryMetadata().returnTileX,
			                                          request.getMemoryMetadata().returnTileY,
			                                          request.getMemoryMetadata().returnChannel,
			                                          0);

			LOKI_LOG << this->name() << " preparing to read " << mPortData[port].WordsLeft << " words from " << LOKI_HEX(mPortData[port].Address) << endl;

			if (mPortData[port].Address + mPortData[port].WordsLeft * 4 > MEMORY_ON_CHIP_SCRATCHPAD_SIZE)
				LOKI_ERROR << this->name() << " fetch request outside valid memory space (address " << mPortData[port].Address << ", length " << (mPortData[port].WordsLeft * 4) << ")" << endl;

			assert((mPortData[port].Address & 0x3) == 0);
			assert(mPortData[port].Address <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert((mPortData[port].WordsLeft * 4) <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert(mPortData[port].Address + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert(mPortData[port].WordsLeft > 0);

			// Send a header flit so the target memory bank knows which data is arriving.
			MemoryMetadata metadata;
			metadata.opcode = STORE_LINE;
			metadata.returnTileX = 2;
			metadata.returnTileY = 0;
			NetworkResponse header(mPortData[port].Address, mPortData[port].ReturnAddress, metadata.flatten());
			mOutputQueues[port].write(header);

			Instrumentation::backgroundMemoryRead(mPortData[port].Address, mPortData[port].WordsLeft);

			// One clock cycle access delay until reading starts

			mPortData[port].State = STATE_READING;
		} else if (request.getMemoryMetadata().opcode == STORE_LINE) {
			mPortData[port].Address = request.payload().toUInt();
			mPortData[port].WordsLeft = CACHE_LINE_WORDS;

			LOKI_LOG << this->name() << " preparing to write " << mPortData[port].WordsLeft << " words to " << LOKI_HEX(mPortData[port].Address) << endl;

			if (mPortData[port].Address + mPortData[port].WordsLeft * 4 > MEMORY_ON_CHIP_SCRATCHPAD_SIZE)
				LOKI_ERROR << this->name() << " store request outside valid memory space (address " << mPortData[port].Address << ", length " << (mPortData[port].WordsLeft * 4) << ")" << endl;

			assert((mPortData[port].Address & 0x3) == 0);
			assert(mPortData[port].Address + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
			assert(mPortData[port].WordsLeft > 0);

			Instrumentation::backgroundMemoryWrite(mPortData[port].Address, mPortData[port].WordsLeft);

			mPortData[port].State = STATE_WRITING;
		} else {
		  throw InvalidOptionException("memory operation", request.getMemoryMetadata().opcode);
		}

		mInputQueues[port].read();
	}
}

void SimplifiedOnChipScratchpad::receiveData(uint port) {
  assert(!mInputQueues[port].full());
  assert(iData[port].valid());

  InputWord newWord;
  newWord.EarliestExecutionCycle = mCycleCounter + (uint64_t)cDelayCycles;
  newWord.Request = iData[port].read();

  LOKI_LOG << this->name() << " received " << newWord.Request.payload() << " (" << memoryOpName(newWord.Request.getMemoryMetadata().opcode) << ")" << endl;

  iData[port].ack();
  mInputQueues[port].write(newWord);
}

void SimplifiedOnChipScratchpad::sendData(uint port) {
  if (mOutputQueues[port].empty())
    next_trigger(mOutputQueues[port].writeEvent());
  else if (oData[port].valid()) {
    LOKI_LOG << this->name() << " is blocked waiting for output to become free" << endl;
    next_trigger(oData[port].ack_event());
  }
  else if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else {
    NetworkResponse response = mOutputQueues[port].read();
    LOKI_LOG << this->name() << " sending " << response << endl;
    oData[port].write(response);
    next_trigger(oData[port].ack_event());
  }
}

void SimplifiedOnChipScratchpad::mainLoop() {
	assert(cBanks <= 16);

  // Process port events

  bool bankAccessed[16];	// Simulate banked memory
  memset(bankAccessed, 0x00, sizeof(bankAccessed));

  for (uint port = 0; port < cPortCount; port++) {
    // Default to non-valid - only last change will become effective

    uint32_t bankSelected = (mPortData[port].Address / 4) % cBanks;

    switch (mPortData[port].State) {
    case STATE_IDLE:
      tryStartRequest(port);
      break;

    case STATE_READING:
      if (!bankAccessed[bankSelected]) {
        uint32_t result = mData[mPortData[port].Address / 4];

        LOKI_LOG << this->name() << " read from " << LOKI_HEX(mPortData[port].Address) << ": " << result << endl;

        mPortData[port].Address += 4;
        mPortData[port].WordsLeft--;

        bool endOfPacket = mPortData[port].WordsLeft == 0;

        NetworkResponse response(result, mPortData[port].ReturnAddress, endOfPacket);
        mOutputQueues[port].write(response);

        if (endOfPacket)
          mPortData[port].State = STATE_IDLE;
//						tryStartRequest(port);

        bankAccessed[bankSelected] = true;
      }

      break;

    case STATE_WRITING:
      if (!bankAccessed[bankSelected] && !mInputQueues[port].empty() && mCycleCounter >= mInputQueues[port].peek().EarliestExecutionCycle) {
        assert(mInputQueues[port].peek().Request.getMemoryMetadata().opcode == PAYLOAD
            || mInputQueues[port].peek().Request.getMemoryMetadata().opcode == PAYLOAD_EOP);

        uint32_t data = mInputQueues[port].read().Request.payload().toUInt();

        LOKI_LOG << this->name() << " wrote to " << LOKI_HEX(mPortData[port].Address) << ": " << data << endl;

        mData[mPortData[port].Address / 4] = data;
        mPortData[port].Address += 4;
        mPortData[port].WordsLeft--;

        if (mPortData[port].WordsLeft == 0)
          tryStartRequest(port);

        bankAccessed[bankSelected] = true;
      }

      break;
    }
  }

  // Advance cycle counter

  mCycleCounter++;
}

SimplifiedOnChipScratchpad::SimplifiedOnChipScratchpad(sc_module_name name, const ComponentID& ID, uint portCount) :
	Component(name, ID),
	cPortCount(portCount),
  mInputQueues(portCount, 1024, "SimplifiedOnChipScratchpad::mInputQueues"), // Virtually infinite queue length
  mOutputQueues(portCount, 1024, "SimplifiedOnChipScratchpad::mOutputQueues")  // Virtually infinite queue length
{
	assert(portCount >= 1);

	cDelayCycles = MEMORY_ON_CHIP_SCRATCHPAD_DELAY;
	cBanks = MEMORY_ON_CHIP_SCRATCHPAD_BANKS;

	iData.init(portCount);
	oData.init(portCount);

	mCycleCounter = 0;

	mData = new uint32_t[MEMORY_ON_CHIP_SCRATCHPAD_SIZE / 4];
	memset(mData, 0x00, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

	mPortData = new PortData[portCount];

	for (uint i = 0; i < portCount; i++)
		mPortData[i].State = STATE_IDLE;

	SC_METHOD(mainLoop);
	sensitive << iClock.pos();
	dont_initialize();

  // Handlers for each of the input queues.
  for (uint i=0; i<portCount; i++)
    SPAWN_METHOD(iData[i], SimplifiedOnChipScratchpad::receiveData, i, false);

  // Handlers for each of the output queues.
  for (uint i=0; i<portCount; i++)
    SPAWN_METHOD(mOutputQueues[i].writeEvent(), SimplifiedOnChipScratchpad::sendData, i, true);
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

					assert(request.getOperation() == STORE_LINE);

					mPortData[port].Address = request.getPayload();
					mPortData[port].WordsLeft = request.getLineSize() / 4;

					if (mPortData[port].Address + mPortData[port].WordsLeft * 4 > MEMORY_ON_CHIP_SCRATCHPAD_SIZE)
						cerr << this->name() << " store request outside valid memory space (address " << mPortData[port].Address << ", length " << (mPortData[port].WordsLeft * 4) << ")" << endl;

					assert((mPortData[port].Address & 0x3) == 0);
					assert(mPortData[port].Address + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
					assert(mPortData[port].WordsLeft > 0);

					Instrumentation::backgroundMemoryWrite(mPortData[port].Address, mPortData[port].WordsLeft);

					mPortData[port].State = STATE_WRITING;

					dataProcessed = true;
				}
			} else {
				assert(!mInputQueues[port].empty());
				assert(mInputQueues[port].peek().Request.getOperation() == PAYLOAD_ONLY);

				mData[mPortData[port].Address / 4] = mInputQueues[port].read().Request.getPayload();
				mPortData[port].Address += 4;
				mPortData[port].WordsLeft--;

				if (mPortData[port].WordsLeft == 0)
					mPortData[port].State = STATE_IDLE;

				dataProcessed = true;
			}
		}
	} while (dataProcessed);
}
*/

void SimplifiedOnChipScratchpad::storeData(vector<Word>& data, MemoryAddr location, bool readOnly) {
	size_t count = data.size();
	uint32_t address = location / 4;

	assert(location % 4 == 0);
	assert(location + count * 4 < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

	for (size_t i = 0; i < count; i++) {
	  LOKI_LOG << this->name() << " wrote to " << LOKI_HEX((address+i)*4) << ": " << data[i].toUInt() << endl;

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
