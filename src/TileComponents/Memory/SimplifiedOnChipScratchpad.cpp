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
#include "../../Memory/BufferArray.h"
#include "../../Utility/Instrumentation.h"
#include "SimplifiedOnChipScratchpad.h"

void SimplifiedOnChipScratchpad::mainLoop() {
	for (;;) {
		// Wait for start of clock cycle

		wait(iClock.posedge_event());

		// Handle input data

		for (uint port = 0; port < mPortCount; port++) {
			if (iDataStrobe[port].read()) {
				InputWord newWord;
				newWord.EarliestExecutionCycle = mCycleCounter + (uint64_t)cDelayCycles;
				newWord.Data = iData[port].read();
				mInputQueues[port].write(newWord);
			}
		}

		// Process port events

		uint accessCount = 0;	// Simulate n-port memory

		for (uint port = 0; port < mPortCount; port++) {
			// Default to non-valid - only last change will become effective

			oDataStrobe[port].write(false);

			switch (mPortData[port].State) {
			case STATE_IDLE:
				if (!mInputQueues[port].empty() && mInputQueues[port].peek().EarliestExecutionCycle >= mCycleCounter) {
					uint32_t word = mInputQueues[port].read().Data.toUInt();
					mPortData[port].WordsLeft = word & 0x7FFFFFFFUL;

					assert(mPortData[port].WordsLeft > 0);

					mPortData[port].State = ((word & 0x80000000UL) != 0) ? STATE_WRITE_ADDRESS : STATE_READ_ADDRESS;
				}

				break;

			case STATE_READ_ADDRESS:
				if (!mInputQueues[port].empty() && mInputQueues[port].peek().EarliestExecutionCycle >= mCycleCounter) {
					uint32_t word = mInputQueues[port].read().Data.toUInt();
					mPortData[port].Address = word;

					assert((mPortData[port].Address & 0x3) == 0);
					assert(mPortData[port].Address + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

					// Do not output first word directly - assume one clock cycle access delay

					mPortData[port].State = STATE_READING;
				}

				break;

			case STATE_READING:
				if (accessCount < cPorts) {
					oDataStrobe[port].write(true);
					oData[port].write(Word(mData[mPortData[port].Address / 4]));

					mPortData[port].Address += 4;
					mPortData[port].WordsLeft--;

					if (mPortData[port].WordsLeft == 0) {
						if (!mInputQueues[port].empty() && mInputQueues[port].peek().EarliestExecutionCycle >= mCycleCounter) {
							uint32_t word = mInputQueues[port].read().Data.toUInt();
							mPortData[port].WordsLeft = word & 0x7FFFFFFFUL;

							assert(mPortData[port].WordsLeft > 0);

							mPortData[port].State = ((word & 0x80000000UL) != 0) ? STATE_WRITE_ADDRESS : STATE_READ_ADDRESS;
						} else {
							mPortData[port].State = STATE_IDLE;
						}
					}

					accessCount++;
				}

				break;

			case STATE_WRITE_ADDRESS:
				if (!mInputQueues[port].empty() && mInputQueues[port].peek().EarliestExecutionCycle >= mCycleCounter) {
					uint32_t word = mInputQueues[port].read().Data.toUInt();
					mPortData[port].Address = word;

					assert((mPortData[port].Address & 0x3) == 0);
					assert(mPortData[port].Address + mPortData[port].WordsLeft * 4 <= MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

					mPortData[port].State = STATE_WRITING;
				}

				break;

			case STATE_WRITING:
				if (accessCount < cPorts && !mInputQueues[port].empty() && mInputQueues[port].peek().EarliestExecutionCycle >= mCycleCounter) {
					mData[mPortData[port].Address / 4] = mInputQueues[port].read().Data.toUInt();
					mPortData[port].Address += 4;
					mPortData[port].WordsLeft--;

					if (mPortData[port].WordsLeft == 0)
						mPortData[port].State = STATE_IDLE;

					accessCount++;
				}

				break;
			}
		}

		// Generate new idle signal

		bool idle = true;

		for (uint port = 0; port < mPortCount; port++) {
			if (mPortData[port].State != STATE_IDLE || !mInputQueues[port].empty()) {
				idle = false;
				break;
			}
		}

		oIdle.write(idle);

		Instrumentation::idle(id, !idle);

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
	cPorts = MEMORY_ON_CHIP_SCRATCHPAD_PORTS;

	oIdle.initialize(true);

	iDataStrobe = new sc_in<bool>[portCount];
	iData = new sc_in<Word>[portCount];
	oDataStrobe = new sc_out<bool>[portCount];
	oData = new sc_out<Word>[portCount];

	for (uint i = 0; i < portCount; i++)
		oDataStrobe[i].initialize(false);

	mCycleCounter = 0;

	mData = new uint32_t[MEMORY_ON_CHIP_SCRATCHPAD_SIZE / 4];
	memset(mData, 0x00, MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

	mPortCount = portCount;
	mPortData = new PortData[portCount];

	for (uint i = 0; i < portCount; i++)
		mPortData[i].State = STATE_IDLE;

	SC_THREAD(mainLoop);

	Instrumentation::idle(id, true);
}

SimplifiedOnChipScratchpad::~SimplifiedOnChipScratchpad() {
	delete[] iDataStrobe;
	delete[] iData;
	delete[] oDataStrobe;
	delete[] oData;
	delete[] mData;
	delete[] mPortData;
}

void SimplifiedOnChipScratchpad::storeData(vector<Word>& data, MemoryAddr location) {
	size_t count = data.size();
	uint32_t address = location / 4;

	assert(location % 4 == 0);
	assert(location + count * 4 < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);

	for (size_t i = 0; i < count; i++) {
		if (DEBUG)
			cout << this->name() << " store 0x" << setprecision(8) << setfill('0') << hex << ((address + i) * 4) << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data[i].toUInt() << endl << dec;

		mData[address + i] = data[i].toUInt();
	}
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
	uint shift = (3 - (addr % 4)) * 8;
	return Word((data >> shift) & 0xFF);
}

void SimplifiedOnChipScratchpad::writeWord(MemoryAddr addr, Word data) {
	assert(addr < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	assert(addr % 4 == 0);
	mData[addr / 4] = data.toUInt();
}

void SimplifiedOnChipScratchpad::writeByte(MemoryAddr addr, Word data) {
	assert(addr < MEMORY_ON_CHIP_SCRATCHPAD_SIZE);
	uint32_t modData = mData[addr / 4];
	uint shift = 3 - (addr % 4);
	uint32_t mask = 0xFF << (shift * 8);
	modData &= ~mask;
	modData |= (data.toUInt() & 0xFF) << shift;
	mData[addr / 4] = modData;
}

double SimplifiedOnChipScratchpad::area() const {
	assert(false);
	return 0.0;
}

double SimplifiedOnChipScratchpad::energy() const {
	assert(false);
	return 0.0;
}
