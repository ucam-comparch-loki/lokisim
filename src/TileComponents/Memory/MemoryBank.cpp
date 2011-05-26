//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Configurable Memory Bank Implementation
//-------------------------------------------------------------------------------------------------
// Second version of configurable memory bank model. Each memory bank is
// directly connected to the network. There are multiple memory banks per tile.
//
// Memory requests are connection-less. Instead, the new channel map table
// mechanism in the memory bank is used.
//
// The number of input and output ports is fixed to 1. The ports possess
// queues for incoming and outgoing data.
//-------------------------------------------------------------------------------------------------
// File:       MemoryBank.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 08/04/2011
//-------------------------------------------------------------------------------------------------

#include <iostream>
#include <iomanip>

using namespace std;

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Instruction.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Memory/BufferStorage.h"
#include "../../Utility/Parameters.h"
#include "GeneralPurposeCacheHandler.h"
#include "ScratchpadModeHandler.h"
#include "SimplifiedOnChipScratchpad.h"
#include "MemoryBank.h"

//-------------------------------------------------------------------------------------------------
// Internal functions
//-------------------------------------------------------------------------------------------------

uint MemoryBank::log2Exact(uint value) {
	assert(value > 1);

	uint result = 0;
	uint temp = value >> 1;

	while (temp != 0) {
		result++;
		temp >>= 1;
	}

	assert(1UL << result == value);

	return result;
}

bool MemoryBank::processRingEvent() {
	if (!mRingRequestInputPending)
		return false;

ReevaluateRequest:

	bool ringRequestConsumed = true;

	switch (mActiveRingRequestInput.Header.RequestType) {
	case RING_SET_MODE:
		if (DEBUG)
			cout << this->name() << " received RING_SET_MODE request through ring network" << endl;

		mWayCount = mActiveRingRequestInput.Header.SetMode.WayCount;
		mLineSize = mActiveRingRequestInput.Header.SetMode.LineSize;

		mGroupBaseBank = mActiveRingRequestInput.Header.SetMode.GroupBaseBank;
		mGroupIndex = mActiveRingRequestInput.Header.SetMode.GroupIndex;
		mGroupSize = mActiveRingRequestInput.Header.SetMode.GroupSize;

		assert(mGroupIndex <= mGroupSize);

		if (mActiveRingRequestInput.Header.SetMode.NewMode == MODE_SCRATCHPAD) {
			mBankMode = MODE_SCRATCHPAD;
			mScratchpadModeHandler.activate(mGroupIndex, mGroupSize, mWayCount, mLineSize);
		} else {
			mBankMode = MODE_GP_CACHE;
			mGeneralPurposeCacheHandler.activate(mGroupIndex, mGroupSize, mWayCount, mLineSize);
		}

		if (mGroupIndex < mGroupSize - 1) {
			// Forward request through ring network

			if (mRingRequestOutputPending) {
				mDelayedRingRequestOutput.Header.RequestType = RING_SET_MODE;
				mDelayedRingRequestOutput.Header.SetMode.NewMode = mBankMode;
				mDelayedRingRequestOutput.Header.SetMode.WayCount = mWayCount;
				mDelayedRingRequestOutput.Header.SetMode.LineSize = mLineSize;
				mDelayedRingRequestOutput.Header.SetMode.GroupBaseBank = mGroupBaseBank;
				mDelayedRingRequestOutput.Header.SetMode.GroupIndex = mGroupIndex + 1;
				mDelayedRingRequestOutput.Header.SetMode.GroupSize = mGroupSize;
				mFSMState = STATE_WAIT_RING_OUTPUT;
			} else {
				mRingRequestOutputPending = true;
				mActiveRingRequestOutput.Header.RequestType = RING_SET_MODE;
				mActiveRingRequestOutput.Header.SetMode.NewMode = mBankMode;
				mActiveRingRequestOutput.Header.SetMode.WayCount = mWayCount;
				mActiveRingRequestOutput.Header.SetMode.LineSize = mLineSize;
				mActiveRingRequestOutput.Header.SetMode.GroupBaseBank = mGroupBaseBank;
				mActiveRingRequestOutput.Header.SetMode.GroupIndex = mGroupIndex + 1;
				mActiveRingRequestOutput.Header.SetMode.GroupSize = mGroupSize;
				mFSMState = STATE_IDLE;
			}
		} else {
			mFSMState = STATE_IDLE;
		}

		break;

	case RING_SET_TABLE_ENTRY:
		if (DEBUG)
			cout << this->name() << " received RING_SET_TABLE_ENTRY request through ring network" << (mOutputQueue.empty() ? "" : " (draining output queue)") << endl;

		// Drain output queue before changing table

		if (!mOutputQueue.empty()) {
			ringRequestConsumed = false;
			mFSMState = STATE_IDLE;
			break;
		}

		mActiveTableIndex = mActiveRingRequestInput.Header.SetTableEntry.TableIndex;
		mChannelMapTable[mActiveTableIndex].Valid = true;
		mChannelMapTable[mActiveTableIndex].ReturnChannel = mActiveRingRequestInput.Header.SetTableEntry.TableEntry;

		if (mGroupIndex < mGroupSize - 1) {
			// Forward request through ring network

			if (mRingRequestOutputPending) {
				mDelayedRingRequestOutput.Header.RequestType = RING_SET_TABLE_ENTRY;
				mDelayedRingRequestOutput.Header.SetTableEntry.TableIndex = mActiveTableIndex;
				mDelayedRingRequestOutput.Header.SetTableEntry.TableEntry = mActiveRingRequestInput.Header.SetTableEntry.TableEntry;
				mFSMState = STATE_WAIT_RING_OUTPUT;
			} else {
				mRingRequestOutputPending = true;
				mActiveRingRequestOutput.Header.RequestType = RING_SET_TABLE_ENTRY;
				mActiveRingRequestOutput.Header.SetTableEntry.TableIndex = mActiveTableIndex;
				mActiveRingRequestOutput.Header.SetTableEntry.TableEntry = mActiveRingRequestInput.Header.SetTableEntry.TableEntry;
				mFSMState = STATE_IDLE;
			}
		} else {
			// Send port claim message to return channel

			OutputWord outWord;
			outWord.TableIndex = mActiveTableIndex;
			outWord.Data = ChannelID(id, mActiveTableIndex);
			outWord.PortClaim = true;
			outWord.LastWord = true;

			mOutputQueue.write(outWord);

			mFSMState = STATE_IDLE;
		}

		break;

	case RING_BURST_READ_HAND_OFF:
		if (DEBUG)
			cout << this->name() << " received RING_BURST_READ_HAND_OFF request through ring network" << endl;

		// A bit of a hack but simplifies the code considerably

		mActiveTableIndex = mActiveRingRequestInput.Header.BurstReadHandOff.TableIndex;
		mActiveRequest = MemoryRequest(MemoryRequest::BURST_READ, 0);
		mActiveAddress = mActiveRingRequestInput.Header.BurstReadHandOff.Address;
		mActiveBurstLength = mActiveRingRequestInput.Header.BurstReadHandOff.Count;

		mFSMState = STATE_LOCAL_BURST_READ;
		break;

	case RING_BURST_WRITE_FORWARD:
		if (DEBUG)
			cout << this->name() << " received RING_BURST_WRITE_FORWARD request through ring network" << endl;

		// A bit of a hack but simplifies the code considerably

		mActiveRequest = MemoryRequest(MemoryRequest::BURST_WRITE, 0);
		mActiveAddress = mActiveRingRequestInput.Header.BurstWriteForward.Address;
		mActiveBurstLength = mActiveRingRequestInput.Header.BurstWriteForward.Count;

		mFSMState = STATE_BURST_WRITE_SLAVE;
		break;

	case RING_IPK_READ_HAND_OFF:
		if (DEBUG)
			cout << this->name() << " received RING_IPK_READ_HAND_OFF request through ring network" << endl;

		// A bit of a hack but simplifies the code considerably

		mActiveTableIndex = mActiveRingRequestInput.Header.IPKReadHandOff.TableIndex;
		mActiveRequest = MemoryRequest(MemoryRequest::IPK_READ, 0);
		mActiveAddress = mActiveRingRequestInput.Header.IPKReadHandOff.Address;
		mPartialInstructionPending = mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionPending;
		mPartialInstructionData = mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionData;

		Instrumentation::memoryInitiateIPKRead(cBankNumber, true);

		mFSMState = STATE_LOCAL_IPK_READ;
		break;

	case RING_PASS_THROUGH:
		if (DEBUG)
			cout << this->name() << " received RING_PASS_THROUGH request through ring network" << endl;

		if (mActiveRingRequestInput.Header.PassThrough.DestinationBankNumber == cBankNumber) {
			// A bit of a hack but simplifies the code considerably

			RingNetworkRequest request = mActiveRingRequestInput;
			mActiveRingRequestInput.Header.RequestType = mActiveRingRequestInput.Header.PassThrough.EnvelopedRequestType;

			switch (mActiveRingRequestInput.Header.PassThrough.EnvelopedRequestType) {
			case RING_BURST_READ_HAND_OFF:
				mActiveRingRequestInput.Header.BurstReadHandOff.Address = request.Header.PassThrough.Address;
				mActiveRingRequestInput.Header.BurstReadHandOff.Count = request.Header.PassThrough.Count;
				mActiveRingRequestInput.Header.BurstReadHandOff.TableIndex = request.Header.PassThrough.TableIndex;
				break;

			case RING_IPK_READ_HAND_OFF:
				mActiveRingRequestInput.Header.IPKReadHandOff.Address = request.Header.PassThrough.Address;
				mActiveRingRequestInput.Header.IPKReadHandOff.TableIndex = request.Header.PassThrough.TableIndex;
				mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionPending = request.Header.PassThrough.PartialInstructionPending;
				mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionData = request.Header.PassThrough.PartialInstructionData;
				break;

			default:
				assert(false);
				break;
			}

			goto ReevaluateRequest;
		} else {
			// Forward request through ring network

			if (mRingRequestOutputPending) {
				mDelayedRingRequestOutput = mActiveRingRequestInput;
				mFSMState = STATE_WAIT_RING_OUTPUT;
			} else {
				mRingRequestOutputPending = true;
				mActiveRingRequestOutput = mActiveRingRequestInput;
				mFSMState = STATE_IDLE;

				// Allow ring network pass-through in parallel to core-to-memory network operation

				mRingRequestInputPending = false;
				return false;
			}
		}

		break;

	default:
		assert(false);
		break;
	}

	mRingRequestInputPending = !ringRequestConsumed;
	return true;
}

bool MemoryBank::processMessageHeader() {
  mFSMState = STATE_IDLE;

  if (mInputQueue.empty())
		return false;

	mActiveTableIndex = mInputQueue.peek().channelID().getChannel();
	mActiveRequest = MemoryRequest(mInputQueue.peek().payload());
	bool inputWordProcessed = true;

	switch (mActiveRequest.getOperation()) {
	case MemoryRequest::CONTROL:
		if (DEBUG)
			cout << this->name() << " received CONTROL request on channel " << mActiveTableIndex << endl;

		if (mChannelMapTable[mActiveTableIndex].FetchPending) {
			if (DEBUG)
				cout << this->name() << " received channel address" << endl;

			if (!mOutputQueue.empty()) {
				cout << this->name() << " output queue not empty" << endl;

				// Drain output queue before changing table

				inputWordProcessed = false;
				mFSMState = STATE_IDLE;
			} else {
				// Update channel map table

				mChannelMapTable[mActiveTableIndex].Valid = true;
				mChannelMapTable[mActiveTableIndex].FetchPending = false;
				mChannelMapTable[mActiveTableIndex].ReturnChannel = mActiveRequest.getChannelID();

				if (mGroupIndex < mGroupSize - 1) {
					// Forward request through ring network

					if (mRingRequestOutputPending) {
						mDelayedRingRequestOutput.Header.RequestType = RING_SET_TABLE_ENTRY;
						mDelayedRingRequestOutput.Header.SetTableEntry.TableIndex = mActiveTableIndex;
						mDelayedRingRequestOutput.Header.SetTableEntry.TableEntry = mActiveRequest.getChannelID();
						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_SET_TABLE_ENTRY;
						mActiveRingRequestOutput.Header.SetTableEntry.TableIndex = mActiveTableIndex;
						mActiveRingRequestOutput.Header.SetTableEntry.TableEntry = mActiveRequest.getChannelID();
						mFSMState = STATE_IDLE;
					}
				} else {
					// Send port claim message to return channel

					OutputWord outWord;
					outWord.TableIndex = mActiveTableIndex;
					outWord.Data = ChannelID(id, mActiveTableIndex);
					outWord.PortClaim = true;
					outWord.LastWord = true;

					mOutputQueue.write(outWord);

					mFSMState = STATE_IDLE;
				}
			}
		} else if (mActiveRequest.getOpCode() == MemoryRequest::SET_MODE) {
			if (DEBUG)
				cout << this->name() << " received SET_MODE opcode on channel " << mActiveTableIndex << endl;

			uint wayBits = mActiveRequest.getWayBits();
			uint lineBits = mActiveRequest.getLineBits();

			mWayCount = wayBits == 0 ? MEMORY_CACHE_WAY_COUNT : (1U << (wayBits - 1));
			mLineSize = lineBits == 0 ? MEMORY_CACHE_LINE_SIZE : (1U << (lineBits - 1));

			mGroupBaseBank = cBankNumber;
			mGroupIndex = 0;
			mGroupSize = 1UL << mActiveRequest.getGroupBits();

			assert(mGroupIndex <= mGroupSize);

			if (mActiveRequest.getMode() == MemoryRequest::SCRATCHPAD) {
				mBankMode = MODE_SCRATCHPAD;
				mScratchpadModeHandler.activate(mGroupIndex, mGroupSize, mWayCount, mLineSize);
			} else {
				mBankMode = MODE_GP_CACHE;
				mGeneralPurposeCacheHandler.activate(mGroupIndex, mGroupSize, mWayCount, mLineSize);
			}

			if (mGroupIndex < mGroupSize - 1) {
				// Forward request through ring network

				if (mRingRequestOutputPending) {
					mDelayedRingRequestOutput.Header.RequestType = RING_SET_MODE;
					mDelayedRingRequestOutput.Header.SetMode.NewMode = mBankMode;
					mDelayedRingRequestOutput.Header.SetMode.WayCount = mWayCount;
					mDelayedRingRequestOutput.Header.SetMode.LineSize = mLineSize;
					mDelayedRingRequestOutput.Header.SetMode.GroupBaseBank = mGroupBaseBank;
					mDelayedRingRequestOutput.Header.SetMode.GroupIndex = mGroupIndex + 1;
					mDelayedRingRequestOutput.Header.SetMode.GroupSize = mGroupSize;
					mFSMState = STATE_WAIT_RING_OUTPUT;
				} else {
					mRingRequestOutputPending = true;
					mActiveRingRequestOutput.Header.RequestType = RING_SET_MODE;
					mActiveRingRequestOutput.Header.SetMode.NewMode = mBankMode;
					mActiveRingRequestOutput.Header.SetMode.WayCount = mWayCount;
					mActiveRingRequestOutput.Header.SetMode.LineSize = mLineSize;
					mActiveRingRequestOutput.Header.SetMode.GroupBaseBank = mGroupBaseBank;
					mActiveRingRequestOutput.Header.SetMode.GroupIndex = mGroupIndex + 1;
					mActiveRingRequestOutput.Header.SetMode.GroupSize = mGroupSize;
					mFSMState = STATE_IDLE;
				}
			} else {
				mFSMState = STATE_IDLE;
			}
		} else {
			assert(mActiveRequest.getOpCode() == MemoryRequest::SET_CHMAP);

			if (DEBUG)
				cout << this->name() << " received SET_CHMAP opcode on channel " << mActiveTableIndex << endl;

			mChannelMapTable[mActiveTableIndex].FetchPending = true;

			mFSMState = STATE_IDLE;
		}

		break;

	case MemoryRequest::LOAD_W:
	case MemoryRequest::LOAD_HW:
	case MemoryRequest::LOAD_B:
	case MemoryRequest::STORE_W:
	case MemoryRequest::STORE_HW:
	case MemoryRequest::STORE_B:
		if (DEBUG)
			cout << this->name() << " received scalar request on channel " << mActiveTableIndex << endl;

		mActiveAddress = mActiveRequest.getPayload();
		mFSMState = STATE_LOCAL_MEMORY_ACCESS;

		break;

	case MemoryRequest::IPK_READ:
		if (DEBUG)
			cout << this->name() << " received IPK_READ request on channel " << mActiveTableIndex << endl;

		Instrumentation::memoryInitiateIPKRead(cBankNumber, false);

		mActiveAddress = mActiveRequest.getPayload();
		mFSMState = STATE_LOCAL_IPK_READ;

		break;

	case MemoryRequest::BURST_READ:
		if (DEBUG)
			cout << this->name() << " received BURST_READ request on channel " << mActiveTableIndex << endl;

		mActiveAddress = mActiveRequest.getPayload();
		mFSMCallbackState = STATE_LOCAL_BURST_READ;
		mFSMState = STATE_FETCH_BURST_LENGTH;

		break;

	case MemoryRequest::BURST_WRITE:
		if (DEBUG)
			cout << this->name() << " received BURST_WRITE request on channel " << mActiveTableIndex << endl;

		mActiveAddress = mActiveRequest.getPayload();
		mFSMCallbackState = STATE_BURST_WRITE_MASTER;
		mFSMState = STATE_FETCH_BURST_LENGTH;

		break;

	default:
		cout << this->name() << " received invalid memory request type (" << mActiveRequest.getOperation() << ")" << endl;
		assert(false);
		break;
	}

	if (inputWordProcessed) {
		mInputQueue.discardTop();
		return true;
	} else {
		return false;
	}
}

void MemoryBank::processFetchBurstLength() {
	if (!mInputQueue.empty()) {
		MemoryRequest request(mInputQueue.read().payload());
		assert(request.getOperation() == MemoryRequest::PAYLOAD_ONLY);
		mActiveBurstLength = request.getPayload();
		mFSMState = mFSMCallbackState;
	}
}

void MemoryBank::processLocalMemoryAccess() {
	assert(mBankMode != MODE_INACTIVE);
	assert(mChannelMapTable[mActiveTableIndex].Valid);

	if (mOutputQueue.full() && (mActiveRequest.getOperation() == MemoryRequest::LOAD_W || mActiveRequest.getOperation() == MemoryRequest::LOAD_HW || mActiveRequest.getOperation() == MemoryRequest::LOAD_B)) {
		// Delay load request until there is room in the output queue available

		return;
	} else if (mBankMode == MODE_SCRATCHPAD) {
		assert(mScratchpadModeHandler.containsAddress(mActiveAddress));

		if (mActiveRequest.getOperation() == MemoryRequest::LOAD_W || mActiveRequest.getOperation() == MemoryRequest::LOAD_HW || mActiveRequest.getOperation() == MemoryRequest::LOAD_B) {
			uint32_t data;

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::LOAD_W:		data = mScratchpadModeHandler.readWord(mActiveAddress, false);	break;
			case MemoryRequest::LOAD_HW:	data = mScratchpadModeHandler.readHalfWord(mActiveAddress);		break;
			case MemoryRequest::LOAD_B:		data = mScratchpadModeHandler.readByte(mActiveAddress);			break;
			default:						assert(false);													break;
			}

			// Enqueue output request

			OutputWord outWord;
			outWord.TableIndex = mActiveTableIndex;
			outWord.Data = Word(data);
			outWord.PortClaim = false;
			outWord.LastWord = true;

			mOutputQueue.write(outWord);
		} else if (mActiveRequest.getOperation() == MemoryRequest::STORE_W || mActiveRequest.getOperation() == MemoryRequest::STORE_HW || mActiveRequest.getOperation() == MemoryRequest::STORE_B) {
			if (mInputQueue.empty())
				return;

			MemoryRequest payload(mInputQueue.read().payload());
			assert(payload.getOperation() == MemoryRequest::PAYLOAD_ONLY);

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::STORE_W:	mScratchpadModeHandler.writeWord(mActiveAddress, payload.getPayload());		break;
			case MemoryRequest::STORE_HW:	mScratchpadModeHandler.writeHalfWord(mActiveAddress, payload.getPayload());	break;
			case MemoryRequest::STORE_B:	mScratchpadModeHandler.writeByte(mActiveAddress, payload.getPayload());		break;
			default:						assert(false);																break;
			}
		} else {
			assert(false);
		}

		// Chain next request

		if (!processRingEvent())
			processMessageHeader();
	} else {
		assert(mGeneralPurposeCacheHandler.containsAddress(mActiveAddress));

		if (mActiveRequest.getOperation() == MemoryRequest::LOAD_W || mActiveRequest.getOperation() == MemoryRequest::LOAD_HW || mActiveRequest.getOperation() == MemoryRequest::LOAD_B) {
			bool cacheHit;
			uint32_t data;

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::LOAD_W:		cacheHit = mGeneralPurposeCacheHandler.readWord(mActiveAddress, data, false, mCacheResumeRequest, false);	break;
			case MemoryRequest::LOAD_HW:	cacheHit = mGeneralPurposeCacheHandler.readHalfWord(mActiveAddress, data, mCacheResumeRequest, false);		break;
			case MemoryRequest::LOAD_B:		cacheHit = mGeneralPurposeCacheHandler.readByte(mActiveAddress, data, mCacheResumeRequest, false);			break;
			default:						assert(false);																								break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			if (cacheHit) {
				// Enqueue output request

				OutputWord outWord;
				outWord.TableIndex = mActiveTableIndex;
				outWord.Data = Word(data);
				outWord.PortClaim = false;
				outWord.LastWord = true;

				mOutputQueue.write(outWord);

				// Chain next request

				if (!processRingEvent())
					processMessageHeader();
			} else {
				mFSMCallbackState = STATE_LOCAL_MEMORY_ACCESS;
				mFSMState = STATE_GP_CACHE_MISS;
				mCacheFSMState = GP_CACHE_STATE_PREPARE;
			}
		} else if (mActiveRequest.getOperation() == MemoryRequest::STORE_W || mActiveRequest.getOperation() == MemoryRequest::STORE_HW || mActiveRequest.getOperation() == MemoryRequest::STORE_B) {
			if (mInputQueue.empty())
				return;

			bool cacheHit;
			MemoryRequest payload(mInputQueue.peek().payload());

			assert(payload.getOperation() == MemoryRequest::PAYLOAD_ONLY);

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::STORE_W:	cacheHit = mGeneralPurposeCacheHandler.writeWord(mActiveAddress, payload.getPayload(), mCacheResumeRequest, false);		break;
			case MemoryRequest::STORE_HW:	cacheHit = mGeneralPurposeCacheHandler.writeHalfWord(mActiveAddress, payload.getPayload(), mCacheResumeRequest, false);	break;
			case MemoryRequest::STORE_B:	cacheHit = mGeneralPurposeCacheHandler.writeByte(mActiveAddress, payload.getPayload(), mCacheResumeRequest, false);		break;
			default:						assert(false);																											break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			if (cacheHit) {
			  mInputQueue.read();

			  // Chain next request

				if (!processRingEvent())
					processMessageHeader();
			} else {
				mFSMCallbackState = STATE_LOCAL_MEMORY_ACCESS;
				mFSMState = STATE_GP_CACHE_MISS;
				mCacheFSMState = GP_CACHE_STATE_PREPARE;
			}
		} else {
			assert(false);
		}
	}
}

void MemoryBank::processLocalIPKRead() {
	assert(mBankMode != MODE_INACTIVE);
	assert(mChannelMapTable[mActiveTableIndex].Valid);
	assert(mActiveRequest.getOperation() == MemoryRequest::IPK_READ);

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available

		return;
	} else if (mBankMode == MODE_SCRATCHPAD) {
		assert(mScratchpadModeHandler.containsAddress(mActiveAddress));

		uint32_t data = mScratchpadModeHandler.readWord(mActiveAddress, true);

		if (mPartialInstructionPending) {
			//TODO: This is broken - but it is broken in a way the code loader depends on (will be cleaned up with new encoding)
			uint64_t fullData = ((uint64_t)mPartialInstructionData << 32) | (uint64_t)data;
			mPartialInstructionPending = false;

			Instruction inst(fullData);
			bool endOfPacket = inst.endOfPacket();

			// Enqueue output request

			OutputWord outWord;
			outWord.TableIndex = mActiveTableIndex;
			outWord.Data = inst;
			outWord.PortClaim = false;
			outWord.LastWord = endOfPacket;

			mOutputQueue.write(outWord);

			// Handle IPK streaming

			if (endOfPacket) {
				// Chain next request

				if (!processRingEvent())
					processMessageHeader();
			} else {
				mActiveAddress += 4;

				if (mScratchpadModeHandler.containsAddress(mActiveAddress)) {
					mFSMState = STATE_LOCAL_IPK_READ;
				} else {
					if (mGroupIndex == mGroupSize - 1) {
						if (mRingRequestOutputPending) {
							mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
							mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mDelayedRingRequestOutput.Header.PassThrough.Count = 0;
							mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
							mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionPending = false;
							mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionData = 0;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.PassThrough.Count = 0;
							mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
							mActiveRingRequestOutput.Header.PassThrough.PartialInstructionPending = false;
							mActiveRingRequestOutput.Header.PassThrough.PartialInstructionData = 0;

							// Chain next request

							if (!processRingEvent())
								processMessageHeader();
						}
					} else {
						if (mRingRequestOutputPending) {
							mDelayedRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = false;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = 0;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
							mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = false;
							mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = 0;

							// Chain next request

							if (!processRingEvent())
								processMessageHeader();
						}
					}
				}
			}
		} else {
			mActiveAddress += 4;

			if (mScratchpadModeHandler.containsAddress(mActiveAddress)) {
				mPartialInstructionPending = true;
				mPartialInstructionData = data;

				mFSMState = STATE_LOCAL_IPK_READ;
			} else {
				if (mGroupIndex == mGroupSize - 1) {
					if (mRingRequestOutputPending) {
						mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
						mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
						mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
						mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
						mDelayedRingRequestOutput.Header.PassThrough.Count = 0;
						mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
						mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionPending = true;
						mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionData = data;

						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
						mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
						mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
						mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
						mActiveRingRequestOutput.Header.PassThrough.Count = 0;
						mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
						mActiveRingRequestOutput.Header.PassThrough.PartialInstructionPending = true;
						mActiveRingRequestOutput.Header.PassThrough.PartialInstructionData = data;

						// Chain next request

						if (!processRingEvent())
							processMessageHeader();
					}
				} else {
					if (mRingRequestOutputPending) {
						mDelayedRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
						mDelayedRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
						mDelayedRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
						mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = true;
						mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = data;

						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
						mActiveRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
						mActiveRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
						mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = true;
						mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = data;

						// Chain next request

						if (!processRingEvent())
							processMessageHeader();
					}
				}
			}
		}
	} else {
		if (DEBUG)
			cout << this->name() << " reading instruction at address " << mActiveAddress << " from local cache" << endl;

		assert(mGeneralPurposeCacheHandler.containsAddress(mActiveAddress));

		uint32_t data;
		bool cacheHit = mGeneralPurposeCacheHandler.readWord(mActiveAddress, data, true, mCacheResumeRequest, false);

		assert(!(mCacheResumeRequest && !cacheHit));
		mCacheResumeRequest = false;

		if (cacheHit) {
			if (mPartialInstructionPending) {
				//TODO: This is broken - but it is broken in a way the code loader depends on (will be cleaned up with new encoding)
				uint64_t fullData = ((uint64_t)mPartialInstructionData << 32) | (uint64_t)data;
				mPartialInstructionPending = false;

				Instruction inst(fullData);
				bool endOfPacket = inst.endOfPacket();

				if (DEBUG)
					cout << this->name() << " cache hit " << (endOfPacket ? "(end of packet reached)" : "(end of packet not reached)") << ": " << inst << endl;

				// Enqueue output request

				OutputWord outWord;
				outWord.TableIndex = mActiveTableIndex;
				outWord.Data = inst;
				outWord.PortClaim = false;
				outWord.LastWord = endOfPacket;

				mOutputQueue.write(outWord);

				// Handle IPK streaming

				if (endOfPacket) {
					// Chain next request

					if (!processRingEvent())
						processMessageHeader();
				} else {
					mActiveAddress += 4;

					if (mGeneralPurposeCacheHandler.containsAddress(mActiveAddress)) {
						mFSMState = STATE_LOCAL_IPK_READ;
					} else {
						if (mGroupIndex == mGroupSize - 1) {
							if (mRingRequestOutputPending) {
								mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
								mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
								mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
								mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
								mDelayedRingRequestOutput.Header.PassThrough.Count = 0;
								mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
								mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionPending = false;
								mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionData = 0;

								mFSMState = STATE_WAIT_RING_OUTPUT;
							} else {
								mRingRequestOutputPending = true;
								mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
								mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
								mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
								mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
								mActiveRingRequestOutput.Header.PassThrough.Count = 0;
								mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
								mActiveRingRequestOutput.Header.PassThrough.PartialInstructionPending = false;
								mActiveRingRequestOutput.Header.PassThrough.PartialInstructionData = 0;

								// Chain next request

								if (!processRingEvent())
									processMessageHeader();
							}
						} else {
							if (mRingRequestOutputPending) {
								mDelayedRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
								mDelayedRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
								mDelayedRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
								mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = false;
								mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = 0;

								mFSMState = STATE_WAIT_RING_OUTPUT;
							} else {
								mRingRequestOutputPending = true;
								mActiveRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
								mActiveRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
								mActiveRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
								mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = false;
								mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = 0;

								// Chain next request

								if (!processRingEvent())
									processMessageHeader();
							}
						}
					}
				}
			} else {
				if (DEBUG)
					cout << this->name() << " cache hit (partial instruction)" << endl;

				mActiveAddress += 4;

				if (mGeneralPurposeCacheHandler.containsAddress(mActiveAddress)) {
					mPartialInstructionPending = true;
					mPartialInstructionData = data;

					mFSMState = STATE_LOCAL_IPK_READ;
				} else {
					if (mGroupIndex == mGroupSize - 1) {
						if (mRingRequestOutputPending) {
							mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
							mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mDelayedRingRequestOutput.Header.PassThrough.Count = 0;
							mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
							mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionPending = true;
							mDelayedRingRequestOutput.Header.PassThrough.PartialInstructionData = data;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.PassThrough.Count = 0;
							mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
							mActiveRingRequestOutput.Header.PassThrough.PartialInstructionPending = true;
							mActiveRingRequestOutput.Header.PassThrough.PartialInstructionData = data;

							// Chain next request

							if (!processRingEvent())
								processMessageHeader();
						}
					} else {
						if (mRingRequestOutputPending) {
							mDelayedRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = true;
							mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = data;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
							mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = true;
							mActiveRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = data;

							// Chain next request

							if (!processRingEvent())
								processMessageHeader();
						}
					}
				}
			}
		} else {
			if (DEBUG)
				cout << this->name() << " cache miss" << endl;

			mFSMCallbackState = STATE_LOCAL_IPK_READ;
			mFSMState = STATE_GP_CACHE_MISS;
			mCacheFSMState = GP_CACHE_STATE_PREPARE;
		}
	}
}

void MemoryBank::processLocalBurstRead() {
	//TODO: Implement burst handling completely
	assert(false);

	assert(mBankMode != MODE_INACTIVE);
	assert(mChannelMapTable[mActiveTableIndex].Valid);
	assert(mActiveRequest.getOperation() == MemoryRequest::BURST_READ);

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available

		return;
	} else if (mBankMode == MODE_SCRATCHPAD) {
		assert(mScratchpadModeHandler.containsAddress(mActiveAddress));

		uint32_t data = mScratchpadModeHandler.readWord(mActiveAddress, false);
		bool endOfPacket = mActiveBurstLength == 1;

		// Enqueue output request

		OutputWord outWord;
		outWord.TableIndex = mActiveTableIndex;
		outWord.Data = data;
		outWord.PortClaim = false;
		outWord.LastWord = endOfPacket;

		mOutputQueue.write(outWord);

		// Handle data streaming

		if (endOfPacket) {
			// Chain next request

			if (!processRingEvent())
				processMessageHeader();
		} else {
			mActiveAddress += 4;
			mActiveBurstLength--;

			if (mScratchpadModeHandler.containsAddress(mActiveAddress)) {
				mFSMState = STATE_LOCAL_BURST_READ;
			} else {
				if (mGroupIndex == mGroupSize - 1) {
					if (mRingRequestOutputPending) {
						mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
						mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
						mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
						mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
						mDelayedRingRequestOutput.Header.PassThrough.Count = mActiveBurstLength;
						mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;

						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
						mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
						mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
						mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
						mActiveRingRequestOutput.Header.PassThrough.Count = mActiveBurstLength;
						mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;

						// Chain next request

						if (!processRingEvent())
							processMessageHeader();
					}
				} else {
					if (mRingRequestOutputPending) {
						mDelayedRingRequestOutput.Header.RequestType = RING_BURST_READ_HAND_OFF;
						mDelayedRingRequestOutput.Header.BurstReadHandOff.Address = mActiveAddress;
						mDelayedRingRequestOutput.Header.BurstReadHandOff.Count = mActiveBurstLength;
						mDelayedRingRequestOutput.Header.BurstReadHandOff.TableIndex = mActiveTableIndex;

						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_BURST_READ_HAND_OFF;
						mActiveRingRequestOutput.Header.BurstReadHandOff.Address = mActiveAddress;
						mActiveRingRequestOutput.Header.BurstReadHandOff.Count = mActiveBurstLength;
						mActiveRingRequestOutput.Header.BurstReadHandOff.TableIndex = mActiveTableIndex;

						// Chain next request

						if (!processRingEvent())
							processMessageHeader();
					}
				}
			}
		}
	} else {
		assert(mGeneralPurposeCacheHandler.containsAddress(mActiveAddress));

		uint32_t data;
		bool cacheHit = mGeneralPurposeCacheHandler.readWord(mActiveAddress, data, false, mCacheResumeRequest, false);

		assert(!(mCacheResumeRequest && !cacheHit));
		mCacheResumeRequest = false;

		if (cacheHit) {
			bool endOfPacket = mActiveBurstLength == 1;

			// Enqueue output request

			OutputWord outWord;
			outWord.TableIndex = mActiveTableIndex;
			outWord.Data = data;
			outWord.PortClaim = false;
			outWord.LastWord = endOfPacket;

			mOutputQueue.write(outWord);

			// Handle data streaming

			if (endOfPacket) {
				// Chain next request

				if (!processRingEvent())
					processMessageHeader();
			} else {
				mActiveAddress += 4;
				mActiveBurstLength--;

				if (mGeneralPurposeCacheHandler.containsAddress(mActiveAddress)) {
					mFSMState = STATE_LOCAL_BURST_READ;
				} else {
					if (mGroupIndex == mGroupSize - 1) {
						if (mRingRequestOutputPending) {
							mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
							mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mDelayedRingRequestOutput.Header.PassThrough.Count = mActiveBurstLength;
							mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.PassThrough.Count = mActiveBurstLength;
							mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;

							// Chain next request

							if (!processRingEvent())
								processMessageHeader();
						}
					} else {
						if (mRingRequestOutputPending) {
							mDelayedRingRequestOutput.Header.RequestType = RING_BURST_READ_HAND_OFF;
							mDelayedRingRequestOutput.Header.BurstReadHandOff.Address = mActiveAddress;
							mDelayedRingRequestOutput.Header.BurstReadHandOff.Count = mActiveBurstLength;
							mDelayedRingRequestOutput.Header.BurstReadHandOff.TableIndex = mActiveTableIndex;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_BURST_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.BurstReadHandOff.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.BurstReadHandOff.Count = mActiveBurstLength;
							mActiveRingRequestOutput.Header.BurstReadHandOff.TableIndex = mActiveTableIndex;

							// Chain next request

							if (!processRingEvent())
								processMessageHeader();
						}
					}
				}
			}
		} else {
			mFSMCallbackState = STATE_LOCAL_BURST_READ;
			mFSMState = STATE_GP_CACHE_MISS;
			mCacheFSMState = GP_CACHE_STATE_PREPARE;
		}
	}
}

void MemoryBank::processBurstWriteMaster() {
	//TODO: Implement handler
	assert(false);
}

void MemoryBank::processBurstWriteSlave() {
	//TODO: Implement handler
	assert(false);
}

void MemoryBank::processGeneralPurposeCacheMiss() {
	// 1. Send command word - highest bit: 0 = read, 1 = write, lower bits word count
	// 2. Send start address
	// 3. Send words in case of write command

	switch (mCacheFSMState) {
	case GP_CACHE_STATE_PREPARE:
		mGeneralPurposeCacheHandler.prepareCacheLine(mActiveAddress, mWriteBackAddress, mWriteBackCount, mCacheLineBuffer, mFetchAddress, mFetchCount);
		mCacheLineCursor = 0;

		if (mWriteBackCount > 0) {
			assert(mWriteBackCount == mLineSize / 4);

			oBMDataStrobe.write(true);
			oBMData.write(MemoryRequest(MemoryRequest::STORE_LINE, mWriteBackAddress, mLineSize));

			mCacheFSMState = GP_CACHE_STATE_SEND_DATA;
		} else {
			assert(mFetchCount == mLineSize / 4);

			oBMDataStrobe.write(true);
			oBMData.write(MemoryRequest(MemoryRequest::FETCH_LINE, mFetchAddress, mLineSize));
			mCacheLineCursor = 0;

			mCacheFSMState = GP_CACHE_STATE_READ_DATA;
		}

		break;

	case GP_CACHE_STATE_SEND_DATA:
		oBMDataStrobe.write(true);
		oBMData.write(MemoryRequest(MemoryRequest::PAYLOAD_ONLY, mCacheLineBuffer[mCacheLineCursor++]));

		mCacheFSMState = (mCacheLineCursor == mWriteBackCount) ? GP_CACHE_STATE_SEND_READ_COMMAND : GP_CACHE_STATE_SEND_DATA;
		break;

	case GP_CACHE_STATE_SEND_READ_COMMAND:
		assert(mFetchCount == mLineSize / 4);

		oBMDataStrobe.write(true);
		oBMData.write(MemoryRequest(MemoryRequest::FETCH_LINE, mFetchAddress, mLineSize));
		mCacheLineCursor = 0;

		mCacheFSMState = GP_CACHE_STATE_READ_DATA;
		break;

	case GP_CACHE_STATE_READ_DATA:
		if (iBMDataStrobe.read())
			mCacheLineBuffer[mCacheLineCursor++] = iBMData.read().toUInt();

		mCacheFSMState = (mCacheLineCursor == mFetchCount) ? GP_CACHE_STATE_REPLACE : GP_CACHE_STATE_READ_DATA;
		break;

	case GP_CACHE_STATE_REPLACE:
		mGeneralPurposeCacheHandler.replaceCacheLine(mFetchAddress, mCacheLineBuffer);

		mCacheResumeRequest = true;
		mFSMState = mFSMCallbackState;
		break;

	default:
		assert(false);
		break;
	}
}

void MemoryBank::processWaitRingOutput() {
	if (!mRingRequestOutputPending) {
		// Write delayed ring output request into ring output buffer

		mRingRequestOutputPending = true;
		mActiveRingRequestOutput = mDelayedRingRequestOutput;

		// Chain next request

		if (!processRingEvent())
			processMessageHeader();
	}
}

void MemoryBank::processValidInput() {
	// Acknowledge new data as soon as it arrives if we know it will be consumed
	// in this clock cycle.
	// FIXME: also needs to update whenever the queue's "full" flag changes.
	oDataInAcknowledge.write(iDataInValid.read() && !mInputQueue.full());
}

void MemoryBank::processValidRing() {
	// Acknowledge new data as soon as it arrives if we know it will be consumed
	// in this clock cycle.
	oRingAcknowledge.write(iRingStrobe.read() && !mRingRequestInputPending);
}

void MemoryBank::handleNetworkInterfacesPre() {
	// Set port defaults

	oDataOutValid.write(false);
	oRingStrobe.write(false);

	// Check whether old output word got acknowledged

	if (mOutputWordPending && iDataOutAcknowledge.read()) {
		if (DEBUG)
			cout << this->name() << " output word got acknowledged" << endl;

		mOutputWordPending = false;
	}

	// Check whether old ring output got acknowledged

	if (mRingRequestOutputPending && iRingAcknowledge.read()) {
		if (DEBUG)
			cout << this->name() << " ring request got acknowledged" << endl;

		mRingRequestOutputPending = false;
	}

	// Check data input ports and update queue

	if (iDataInValid.read() && !mInputQueue.full()) {
		// Acknowledgement handled by separate methods

		mInputQueue.write(iDataIn.read());
	}

	// Check ring input ports and update request registers

	if (iRingStrobe.read() && !mRingRequestInputPending) {
		// Acknowledgement handled by separate methods

		mRingRequestInputPending = true;
		mActiveRingRequestInput = iRingRequest.read();
	}
}

void MemoryBank::handleNetworkInterfacesPost() {
	// Send ring output if possible

	if (mRingRequestOutputPending) {
		oRingStrobe.write(true);
		oRingRequest.write(mActiveRingRequestOutput);
	}

	// Try to send new output word if available

	if (!mOutputWordPending && !mOutputQueue.empty()) {
		OutputWord outWord = mOutputQueue.read();

		AddressedWord word(outWord.Data, mChannelMapTable[outWord.TableIndex].ReturnChannel);
		word.setPortClaim(outWord.PortClaim, false);
		word.setEndOfPacket(outWord.LastWord);
		mOutputWordPending = true;
		mActiveOutputWord = word;
	}

	// Update ports

	if (mOutputWordPending) {
		oDataOutValid.write(true);
		oDataOut.write(mActiveOutputWord);
	}
}

void MemoryBank::mainLoop() {
	for (;;) {
		wait(iClock.posedge_event());

		handleNetworkInterfacesPre();

		// Set output port defaults - only final write will be effective

		oBMDataStrobe.write(false);

		// Proceed according to FSM state

		switch (mFSMState) {
		case STATE_IDLE:
			if (!processRingEvent())
				processMessageHeader();

			break;

		case STATE_FETCH_BURST_LENGTH:
			processFetchBurstLength();
			break;

		case STATE_LOCAL_MEMORY_ACCESS:
			processLocalMemoryAccess();
			break;

		case STATE_LOCAL_IPK_READ:
			processLocalIPKRead();
			break;

		case STATE_LOCAL_BURST_READ:
			processLocalBurstRead();
			break;

		case STATE_BURST_WRITE_MASTER:
			processBurstWriteMaster();
			break;

		case STATE_BURST_WRITE_SLAVE:
			processBurstWriteSlave();
			break;

		case STATE_GP_CACHE_MISS:
			processGeneralPurposeCacheMiss();
			break;

		case STATE_WAIT_RING_OUTPUT:
			processWaitRingOutput();
			break;

		default:
			assert(false);
			break;
		}

		// Send output if possible

		handleNetworkInterfacesPost();

		// Update status signals

		bool idle = mFSMState == STATE_IDLE && mInputQueue.empty() && mOutputQueue.empty() && !mOutputWordPending && !mRingRequestInputPending && !mRingRequestOutputPending;
		oIdle.write(idle);
		Instrumentation::idle(id, idle);
	}
}

//-------------------------------------------------------------------------------------------------
// Constructors and destructors
//-------------------------------------------------------------------------------------------------

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber) :
	Component(name, ID),
	mInputQueue(CHANNEL_END_BUFFER_SIZE, "MemoryBank::mInputQueue"),
	mOutputQueue(CHANNEL_END_BUFFER_SIZE, "MemoryBank::mOutputQueue"),
	mScratchpadModeHandler(bankNumber),
	mGeneralPurposeCacheHandler(bankNumber)
{
	//-- Configuration parameters -----------------------------------------------------------------

	cChannelMapTableEntries = MEMORY_CHANNEL_MAP_TABLE_ENTRIES;
	cMemoryBanks = MEMS_PER_TILE;
	cBankNumber = bankNumber;
	mWayCount = MEMORY_CACHE_WAY_COUNT;
	mLineSize = MEMORY_CACHE_LINE_SIZE;
	cRandomReplacement = MEMORY_CACHE_RANDOM_REPLACEMENT != 0;

	assert(cChannelMapTableEntries > 0);
	assert(cMemoryBanks > 0);
	assert(cBankNumber < cMemoryBanks);

	//-- Data queue state -------------------------------------------------------------------------

	mOutputWordPending = false;

	//-- Mode independent state -------------------------------------------------------------------

	mGroupBaseBank = 0;
	mGroupIndex = 0;
	mGroupSize = 0;

	mChannelMapTable = new ChannelMapTableEntry[cChannelMapTableEntries];
	memset(mChannelMapTable, 0x00, sizeof(ChannelMapTableEntry) * cChannelMapTableEntries);

	mBankMode = MODE_INACTIVE;
	mFSMState = STATE_IDLE;
	mFSMCallbackState = STATE_IDLE;

	mPartialInstructionPending = false;
	mPartialInstructionData = 0;

	//-- Cache mode state -------------------------------------------------------------------------

	mCacheResumeRequest = false;

	//-- Ring network state -----------------------------------------------------------------------

	mRingRequestInputPending = false;
	mRingRequestOutputPending = false;

	//-- Debug utilities --------------------------------------------------------------------------

	mPrevMemoryBank = 0;
	mNextMemoryBank = 0;
	mBackgroundMemory = 0;

	//-- Port initialization ----------------------------------------------------------------------

	oIdle.initialize(true);
	oDataInAcknowledge.initialize(false);
	oDataOutValid.initialize(false);
	oBMDataStrobe.initialize(false);
	oRingAcknowledge.initialize(false);
	oRingStrobe.initialize(false);

	//-- Register module with SystemC simulation kernel -------------------------------------------

	Instrumentation::idle(id, true);

	SC_THREAD(mainLoop);

	SC_METHOD(processValidInput);
	sensitive << iDataInValid << iClock.neg();
	// do initialise

	SC_METHOD(processValidRing);
	sensitive << iRingStrobe << iClock.neg();
	// do initialise

	end_module(); // Needed because we're using a different Component constructor
}

MemoryBank::~MemoryBank() {
	// Nothing
}

void MemoryBank::setAdjacentMemories(MemoryBank *prevMemoryBank, MemoryBank *nextMemoryBank, SimplifiedOnChipScratchpad *backgroundMemory) {
	mPrevMemoryBank = prevMemoryBank;
	mNextMemoryBank = nextMemoryBank;
	mBackgroundMemory = backgroundMemory;
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	size_t count = data.size();
	uint32_t address = location;

	assert(address % 4 == 0);
	assert(mBankMode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		for (size_t i = 0; i < count; i++) {
			MemoryBank *bank = firstBank;

			for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
				if (bank->mScratchpadModeHandler.containsAddress(address + i * 4)) {
					bank->mScratchpadModeHandler.writeWord(address + i * 4, data[i].toUInt());
					break;
				}

				bank = bank->mNextMemoryBank;
			}
		}
	} else {
		for (size_t i = 0; i < count; i++) {
			MemoryBank *bank = firstBank;

			for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
				if (bank->mGeneralPurposeCacheHandler.containsAddress(address + i * 4)) {
					if (!bank->mGeneralPurposeCacheHandler.writeWord(address + i * 4, data[i].toUInt(), false, true))
						mBackgroundMemory->writeWord(address + i * 4, data[i]);

					break;
				}

				bank = bank->mNextMemoryBank;
			}
		}
	}
}

void MemoryBank::print(MemoryAddr start, MemoryAddr end) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	if (start > end) {
		MemoryAddr temp = start;
		start = end;
		end = temp;
	}

	size_t address = (start / 4) * 4;
	size_t limit = (end / 4) * 4 + 4;

	assert(mBankMode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		cout << "Virtual memory group scratchpad data (banks " << (cBankNumber - mGroupIndex) << " to " << (cBankNumber - mGroupIndex + mGroupSize - 1) << "):\n" << endl;

		while (address < limit) {
			MemoryBank *bank = firstBank;

			for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
				if (bank->mScratchpadModeHandler.containsAddress(address)) {
					uint32_t data = bank->mScratchpadModeHandler.readWord(address, false);
					cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << endl << dec;
					break;
				}

				bank = bank->mNextMemoryBank;
			}

			address += 4;
		}
	} else {
		cout << "Virtual memory group general purpose cache data (banks " << (cBankNumber - mGroupIndex) << " to " << (cBankNumber - mGroupIndex + mGroupSize - 1) << "):\n" << endl;

		while (address < limit) {
			MemoryBank *bank = firstBank;

			for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
				if (bank->mGeneralPurposeCacheHandler.containsAddress(address)) {
					uint32_t data;
					bool cached = bank->mGeneralPurposeCacheHandler.readWord(address, data, false, false, true);

					if (!cached)
						data = mBackgroundMemory->readWord(address).toUInt();

					cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << dec;
					if (cached)
						cout << " C";
					cout << endl;

					break;
				}

				bank = bank->mNextMemoryBank;
			}

			address += 4;
		}
	}
}

Word MemoryBank::readWord(MemoryAddr addr) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(addr % 4 == 0);
	assert(mBankMode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr))
				return Word(bank->mScratchpadModeHandler.readWord(addr, false));

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return Word(0);
	} else {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mGeneralPurposeCacheHandler.containsAddress(addr)) {
				uint32_t data;
				bool cached = bank->mGeneralPurposeCacheHandler.readWord(addr, data, false, false, true);

				if (!cached)
					data = mBackgroundMemory->readWord(addr).toUInt();

				return Word(data);
			}

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return Word(0);
	}
}

Word MemoryBank::readByte(MemoryAddr addr) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(mBankMode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr))
				return Word(bank->mScratchpadModeHandler.readByte(addr));

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return Word(0);
	} else {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mGeneralPurposeCacheHandler.containsAddress(addr)) {
				uint32_t data;
				bool cached = bank->mGeneralPurposeCacheHandler.readByte(addr, data, false, true);

				if (!cached)
					data = mBackgroundMemory->readByte(addr).toUInt();

				return Word(data);
			}

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return Word(0);
	}
}

void MemoryBank::writeWord(MemoryAddr addr, Word data) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(addr % 4 == 0);
	assert(mBankMode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr)) {
				bank->mScratchpadModeHandler.writeWord(addr, data.toUInt());
				return;
			}

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return;
	} else {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mGeneralPurposeCacheHandler.containsAddress(addr)) {
				bool cached = bank->mGeneralPurposeCacheHandler.writeWord(addr, data.toUInt(), false, true);

				if (!cached)
					mBackgroundMemory->writeWord(addr, data.toUInt());

				return;
			}

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return;
	}
}

void MemoryBank::writeByte(MemoryAddr addr, Word data) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(mBankMode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr)) {
				bank->mScratchpadModeHandler.writeByte(addr, data.toUInt());
				return;
			}

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return;
	} else {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mGeneralPurposeCacheHandler.containsAddress(addr)) {
				bool cached = bank->mGeneralPurposeCacheHandler.writeByte(addr, data.toUInt(), false, true);

				if (!cached)
					mBackgroundMemory->writeByte(addr, data.toUInt());

				return;
			}

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return;
	}
}

double MemoryBank::area() const {
	assert(false);
	return 0.0;
}

double MemoryBank::energy() const {
	assert(false);
	return 0.0;
}
