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
#include "../../Datatype/Instruction.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Memory/BufferStorage.h"
#include "../../Network/Topologies/LocalNetwork.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../../Utility/Trace/MemoryTrace.h"
#include "../../Utility/Parameters.h"
#include "GeneralPurposeCacheHandler.h"
#include "ScratchpadModeHandler.h"
#include "SimplifiedOnChipScratchpad.h"
#include "MemoryBank.h"
#include "../../Exceptions/UnsupportedFeatureException.h"
#include "../../Exceptions/InvalidOptionException.h"

// A "fast" memory is capable of receiving a request, decoding it, performing
// the operation, and sending the result, all in one clock cycle.
// A "slow" memory receives and decodes the request in one cycle, and performs
// it on the next.
#define FAST_MEMORY (true)

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

uint MemoryBank::homeTile(MemoryAddr address) {
  // TODO: access directory

  // Total cache on tile = 64kB = 2^16B
  return (address >> 16) % NUM_COMPUTE_TILES;
}

const NetworkData MemoryBank::peekNextRequest() {
  assert(!mInputReqQueue.empty() || !mInputQueue.empty());

  switch (mCurrentInput) {
    case INPUT_NONE:
      if (!mInputReqQueue.empty()) {
        mCurrentInput = INPUT_MEMORIES;
        return mInputReqQueue.peek();
      }
      else {
        mCurrentInput = INPUT_CORES;
        return mInputQueue.peek();
      }
    case INPUT_MEMORIES:
      return mInputReqQueue.peek();
    case INPUT_CORES:
      return mInputQueue.peek();
    default:
      throw InvalidOptionException("MemoryBank input queue", mCurrentInput);
      return NetworkData();
  }
}

const NetworkData MemoryBank::readNextRequest() {
  // Use the peek method to handle the NONE state and perform any assertions.
  NetworkData result = peekNextRequest();

  // Dequeue the item from its queue.
  if (mCurrentInput == INPUT_MEMORIES)
    mInputReqQueue.read();
  else if (mCurrentInput == INPUT_CORES) {
    mInputQueue.read();
    if (DEBUG)
      cout << this->name() << " dequeued " << result << endl;
  }

  if (result.endOfPacket())
    mCurrentInput = INPUT_NONE;

  return result;
}

bool MemoryBank::currentInputEmpty() {
  switch (mCurrentInput) {
    case INPUT_NONE:
      return mInputReqQueue.empty() && mInputQueue.empty();
    case INPUT_MEMORIES:
      return mInputReqQueue.empty();
    case INPUT_CORES:
      return mInputQueue.empty();
    default:
      throw InvalidOptionException("MemoryBank input queue", mCurrentInput);
      return true;
  }
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
		mActiveReturnChannel = mActiveRingRequestInput.Header.BurstReadHandOff.ReturnChannel;
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
		mActiveReturnChannel = mActiveRingRequestInput.Header.IPKReadHandOff.ReturnChannel;
		mActiveRequest = MemoryRequest(MemoryRequest::IPK_READ, 0);
		mActiveAddress = mActiveRingRequestInput.Header.IPKReadHandOff.Address;
		mPartialInstructionPending = mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionPending;
		mPartialInstructionData = mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionData;

		if (ENERGY_TRACE)
		  Instrumentation::MemoryBank::initiateIPKRead(cBankNumber, true);

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
				mActiveRingRequestInput.Header.BurstReadHandOff.ReturnChannel = request.Header.PassThrough.ReturnChannel;
				break;

			case RING_IPK_READ_HAND_OFF:
				mActiveRingRequestInput.Header.IPKReadHandOff.Address = request.Header.PassThrough.Address;
				mActiveRingRequestInput.Header.IPKReadHandOff.TableIndex = request.Header.PassThrough.TableIndex;
				mActiveRingRequestInput.Header.IPKReadHandOff.ReturnChannel = request.Header.PassThrough.ReturnChannel;
				mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionPending = request.Header.PassThrough.PartialInstructionPending;
				mActiveRingRequestInput.Header.IPKReadHandOff.PartialInstructionData = request.Header.PassThrough.PartialInstructionData;
				break;

			default:
			  throw InvalidOptionException("ring network request type",
			      mActiveRingRequestInput.Header.PassThrough.EnvelopedRequestType);
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
	  throw InvalidOptionException("ring network request type",
	      mActiveRingRequestInput.Header.RequestType);
		break;
	}

	mRingRequestInputPending = !ringRequestConsumed;
	return true;
}

bool MemoryBank::processMessageHeader() {
  mFSMState = STATE_IDLE;

  if (currentInputEmpty())
		return false;

  NetworkData request = peekNextRequest();
    
	mActiveTableIndex = request.channelID().getChannel();
	mActiveReturnChannel = request.returnAddr();
	mActiveRequest = MemoryRequest(request.payload());
	bool inputWordProcessed = true;

	switch (mActiveRequest.getOperation()) {
	case MemoryRequest::CONTROL:
		if (DEBUG)
			cout << this->name() << " starting CONTROL request on channel " << mActiveTableIndex << endl;

		if (mChannelMapTable[mActiveTableIndex].FetchPending) {
			if (DEBUG)
				cout << this->name() << " parsed channel address" << endl;

			if (!mOutputQueue.empty()) {
			  if (DEBUG)
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
				cout << this->name() << " starting SET_MODE opcode on channel " << mActiveTableIndex << endl;

			uint wayBits = mActiveRequest.getWayBits();
			uint lineBits = mActiveRequest.getLineBits();

			mWayCount = 1U << wayBits;
			mLineSize = 1U << lineBits;

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

			// Print a description of the configuration, so we can more-easily tell
			// whether cores are acting consistently.
			if (DEBUG || Arguments::summarise()) {
			  uint tile = id.getTile();
			  uint startBank = mGroupBaseBank;
			  uint endBank = startBank + mGroupSize - 1;
			  stringstream mode;
			  if (mBankMode == MODE_SCRATCHPAD)
			    mode << "scratchpad";
			  else
			    mode << "cache (associativity " << mWayCount << ")";
			  uint lineSize = mLineSize;

			  cout << "MEMORY CONFIG: tile " << tile << ", banks " << startBank << "-"
			      << endBank << ", line size " << lineSize << ", " << mode.str() << endl;
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
				cout << this->name() << " starting SET_CHMAP opcode on channel " << mActiveTableIndex << endl;

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
  case MemoryRequest::LOAD_THROUGH_W:
  case MemoryRequest::LOAD_THROUGH_HW:
  case MemoryRequest::LOAD_THROUGH_B:
  case MemoryRequest::STORE_THROUGH_W:
  case MemoryRequest::STORE_THROUGH_HW:
  case MemoryRequest::STORE_THROUGH_B:
		if (DEBUG)
			cout << this->name() << " starting scalar request on channel " << mActiveTableIndex << endl;

		mActiveAddress = mActiveRequest.getPayload();
		mFSMState = STATE_LOCAL_MEMORY_ACCESS;

		break;

	case MemoryRequest::IPK_READ:
		if (DEBUG)
			cout << this->name() << " starting IPK_READ request on channel " << mActiveTableIndex << endl;

		if (ENERGY_TRACE)
		  Instrumentation::MemoryBank::initiateIPKRead(cBankNumber, false);

		mActiveAddress = mActiveRequest.getPayload();
		mFSMState = STATE_LOCAL_IPK_READ;

		break;

	case MemoryRequest::BURST_READ:
		if (DEBUG)
			cout << this->name() << " starting BURST_READ request on channel " << mActiveTableIndex << endl;

		mActiveAddress = mActiveRequest.getPayload();
		mFSMCallbackState = STATE_LOCAL_BURST_READ;
		mFSMState = STATE_FETCH_BURST_LENGTH;

		break;

	case MemoryRequest::BURST_WRITE:
		if (DEBUG)
			cout << this->name() << " starting BURST_WRITE request on channel " << mActiveTableIndex << endl;

		mActiveAddress = mActiveRequest.getPayload();
		mFSMCallbackState = STATE_BURST_WRITE_MASTER;
		mFSMState = STATE_FETCH_BURST_LENGTH;

		break;

	default:
	  throw InvalidOptionException("memory operation", mActiveRequest.getOperation());
		break;
	}

	if (inputWordProcessed) {
	  readNextRequest();  // We'd already peeked at the request - now discard it.
		return true;
	} else {
		return false;
	}
}

void MemoryBank::processFetchBurstLength() {
	if (!currentInputEmpty()) {
		MemoryRequest request(readNextRequest().payload());
		assert(request.getOperation() == MemoryRequest::PAYLOAD_ONLY);
		mActiveBurstLength = request.getPayload();
		mFSMState = mFSMCallbackState;
	}
}

void MemoryBank::processLocalMemoryAccess() {
	assert(mBankMode != MODE_INACTIVE);
	assert(mActiveTableIndex < 8 || mChannelMapTable[mActiveTableIndex].Valid);

	uint core = mActiveTableIndex + id.getTile()*CORES_PER_TILE;
	uint channel = mActiveReturnChannel;

	if (mOutputQueue.full() && (mActiveRequest.isSingleLoad())) {
		// Delay load request until there is room in the output queue available

		if (DEBUG)
			cout << this->name() << " delayed scalar request due to full output queue" << endl;

		return;
	} else if (mBankMode == MODE_SCRATCHPAD) {
		assert(mScratchpadModeHandler.containsAddress(mActiveAddress));

		if (mActiveRequest.isSingleLoad()) {
			uint32_t data;

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::LOAD_W:		data = mScratchpadModeHandler.readWord(mActiveAddress, false, core, channel);	break;
			case MemoryRequest::LOAD_HW:	data = mScratchpadModeHandler.readHalfWord(mActiveAddress, core, channel);		break;
			case MemoryRequest::LOAD_B:		data = mScratchpadModeHandler.readByte(mActiveAddress, core, channel);			break;
			default: throw InvalidOptionException("memory load operation", mActiveRequest.getOperation()); break;
			}

			if (DEBUG) {
				switch (mActiveRequest.getOperation()) {
				case MemoryRequest::LOAD_W:
					cout << this->name() << " read word " << data << " from 0x" << std::hex << mActiveAddress << std::dec << endl;
					break;
				case MemoryRequest::LOAD_HW:
					cout << this->name() << " read halfword " << data << " from 0x" << std::hex << mActiveAddress << std::dec << endl;
					break;
				case MemoryRequest::LOAD_B:
					cout << this->name() << " read byte " << data << " from 0x" << std::hex << mActiveAddress << std::dec << endl;
					break;
				default:
				  throw InvalidOptionException("memory load operation", mActiveRequest.getOperation());
					break;
				}
			}

			// Enqueue output request

			OutputWord outWord;
			outWord.TableIndex = mActiveTableIndex;
			outWord.ReturnChannel = mActiveReturnChannel;
			outWord.Data = Word(data);
			outWord.PortClaim = false;
			outWord.LastWord = true;

			mOutputQueue.write(outWord);
		} else if (mActiveRequest.isSingleStore()) {
			if (currentInputEmpty())
				return;

			MemoryRequest payload(readNextRequest().payload());
			assert(payload.getOperation() == MemoryRequest::PAYLOAD_ONLY);

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::STORE_W:	mScratchpadModeHandler.writeWord(mActiveAddress, payload.getPayload(), core, channel);		break;
			case MemoryRequest::STORE_HW:	mScratchpadModeHandler.writeHalfWord(mActiveAddress, payload.getPayload(), core, channel);	break;
			case MemoryRequest::STORE_B:	mScratchpadModeHandler.writeByte(mActiveAddress, payload.getPayload(), core, channel);		break;
			default: throw InvalidOptionException("memory store operation", mActiveRequest.getOperation()); break;
			}

			if (DEBUG) {
				switch (mActiveRequest.getOperation()) {
				case MemoryRequest::STORE_W:
					cout << this->name() << " wrote word " << payload.getPayload() << " to 0x" << std::hex << mActiveAddress << std::dec << endl;
					break;
				case MemoryRequest::STORE_HW:
					cout << this->name() << " wrote halfword " << payload.getPayload() << " to 0x" << std::hex << mActiveAddress << std::dec << endl;
					break;
				case MemoryRequest::STORE_B:
					cout << this->name() << " wrote byte " << payload.getPayload() << " to 0x" << std::hex << mActiveAddress << std::dec << endl;
					break;
				default:
				  throw InvalidOptionException("memory store operation", mActiveRequest.getOperation());
					break;
				}
			}
		} else {
			assert(false);
		}

		// Chain next request

		if (!processRingEvent())
			processMessageHeader();
	} else {
		assert(mGeneralPurposeCacheHandler.containsAddress(mActiveAddress));

		if (mActiveRequest.isSingleLoad()) {
			bool cacheHit;
			uint32_t data;

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::LOAD_W:		cacheHit = mGeneralPurposeCacheHandler.readWord(mActiveAddress, data, false, mCacheResumeRequest, false, core, channel);	break;
			case MemoryRequest::LOAD_HW:	cacheHit = mGeneralPurposeCacheHandler.readHalfWord(mActiveAddress, data, mCacheResumeRequest, false, core, channel);		break;
			case MemoryRequest::LOAD_B:		cacheHit = mGeneralPurposeCacheHandler.readByte(mActiveAddress, data, mCacheResumeRequest, false, core, channel);			break;

			// FIXME: temporary hack. Desired code is commented out below.
			case MemoryRequest::LOAD_THROUGH_W:
        cacheHit = true;
        data = mBackgroundMemory->readWord(mActiveAddress).toUInt();
        mActiveRequest.clearThroughAccess();
        break;
      case MemoryRequest::LOAD_THROUGH_HW:
        cacheHit = true;
        data = mBackgroundMemory->readWord(mActiveAddress & ~3).toUInt();
        if (mActiveAddress & 2)
          data = (data >> 16) & 0xFFFF;
        else
          data = data & 0xFFFF;
        mActiveRequest.clearThroughAccess();
        break;
      case MemoryRequest::LOAD_THROUGH_B:
        cacheHit = true;
        data = mBackgroundMemory->readByte(mActiveAddress).toUInt();
        mActiveRequest.clearThroughAccess();
        break;
/*
			case MemoryRequest::LOAD_THROUGH_W:
			case MemoryRequest::LOAD_THROUGH_HW:
			case MemoryRequest::LOAD_THROUGH_B:
			  cacheHit = false;
			  mActiveRequest.clearThroughAccess();  // Perform a normal load when the data arrives
			  break;
*/
			default: throw InvalidOptionException("memory load operation", mActiveRequest.getOperation()); break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			if (cacheHit) {
				if (DEBUG) {
					switch (mActiveRequest.getOperation()) {
					case MemoryRequest::LOAD_W:
						cout << this->name() << " read word " << data << " from 0x" << std::hex << mActiveAddress << std::dec << endl;
						break;
					case MemoryRequest::LOAD_HW:
						cout << this->name() << " read halfword " << data << " from 0x" << std::hex << mActiveAddress << std::dec << endl;
						break;
					case MemoryRequest::LOAD_B:
						cout << this->name() << " read byte " << data << " from 0x" << std::hex << mActiveAddress << std::dec << endl;
						break;
					default:
					  throw InvalidOptionException("memory load operation", mActiveRequest.getOperation());
						break;
					}
				}

				// Enqueue output request

				OutputWord outWord;
				outWord.TableIndex = mActiveTableIndex;
	      outWord.ReturnChannel = mActiveReturnChannel;
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
		} else if (mActiveRequest.isSingleStore()) {
			if (currentInputEmpty())
				return;

			bool cacheHit;
			MemoryRequest payload(peekNextRequest().payload());

/*
			// We will need to also access the data's home tile if we are in store-
			// through mode, and this isn't already the data's home tile.
			bool throughAccess = mActiveRequest.isThroughAccess()
			                   && !(homeTile(mActiveAddress) == id.getTile()); // temp-removal

			// TODO: write-through mode stuff. This works, but is incomplete.
      // Make the request to the next cache in the chain, if necessary.
      // It doesn't matter that we're writing to the buffer twice in one cycle
      // because the buffer is still limited to draining one item per cycle.
      if (throughAccess) {
        // Access the same bank on the data's home tile.
        ChannelID netAddress(homeTile(mActiveAddress), id.getPosition()-CORES_PER_TILE, 0);

        AddressedWord flit1(mActiveRequest, netAddress);
        flit1.setEndOfPacket(false);
        AddressedWord flit2(payload, netAddress);
        flit2.setEndOfPacket(true);
        mOutputReqQueue.write(flit1);
        mOutputReqQueue.write(flit2);
      }
*/

			if (payload.getOperation() != MemoryRequest::PAYLOAD_ONLY) {
				cout << "!!!! " << mActiveRequest.getOperation() << " | " << mActiveAddress << " | " << payload.getOperation() << " | " << payload.getPayload() << endl;
			}

			// The received word should be the continuation of the previous operation
			// (and should come from the same source core?).
//			assert(mActiveRequest.getChannelID().getComponentID().getPosition() == payload.getChannelID().getComponentID().getPosition());
			assert(payload.getOperation() == MemoryRequest::PAYLOAD_ONLY);

			switch (mActiveRequest.getOperation()) {
//			case MemoryRequest::STORE_THROUGH_W:
			case MemoryRequest::STORE_W:	cacheHit = mGeneralPurposeCacheHandler.writeWord(mActiveAddress, payload.getPayload(), mCacheResumeRequest, false, core, channel);		break;
//			case MemoryRequest::STORE_THROUGH_HW:
			case MemoryRequest::STORE_HW:	cacheHit = mGeneralPurposeCacheHandler.writeHalfWord(mActiveAddress, payload.getPayload(), mCacheResumeRequest, false, core, channel);	break;
//			case MemoryRequest::STORE_THROUGH_B:
			case MemoryRequest::STORE_B:	cacheHit = mGeneralPurposeCacheHandler.writeByte(mActiveAddress, payload.getPayload(), mCacheResumeRequest, false, core, channel);		break;

			// FIXME: temporary hack. Would prefer to use commented out code both
			// above and below.
      case MemoryRequest::STORE_THROUGH_W:
        cacheHit = true;
        mBackgroundMemory->writeWord(mActiveAddress, payload.getPayload());
        break;
      case MemoryRequest::STORE_THROUGH_HW: {
        cacheHit = true;
        uint32_t olddata = mBackgroundMemory->readWord(mActiveAddress & ~3).toUInt();
        uint32_t newdata;
        if (mActiveAddress & 2)
          newdata = ((payload.getPayload() & 0xFFFF) << 16) | (olddata & 0xFFFF);
        else
          newdata = ((olddata & 0xFFFF0000)) | (payload.getPayload() & 0xFFFF);

        mBackgroundMemory->writeWord(mActiveAddress & ~3, newdata);
        break;
      }
      case MemoryRequest::STORE_THROUGH_B:
        cacheHit = true;
        mBackgroundMemory->writeByte(mActiveAddress, payload.getPayload());
        break;

			default: throw InvalidOptionException("memory store operation", mActiveRequest.getOperation()); break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			// If this is a through access, we have to write the word back to the
			// next level of cache.
			//cacheHit = cacheHit && !throughAccess;

			if (cacheHit) {
				if (DEBUG) {
					switch (mActiveRequest.getOperation()) {
					case MemoryRequest::STORE_W:
					case MemoryRequest::STORE_THROUGH_W:
						cout << this->name() << " wrote word " << payload.getPayload() << " to 0x" << std::hex << mActiveAddress << std::dec << endl;
						break;
					case MemoryRequest::STORE_HW:
					case MemoryRequest::STORE_THROUGH_HW:
						cout << this->name() << " wrote halfword " << payload.getPayload() << " to 0x" << std::hex << mActiveAddress << std::dec << endl;
						break;
					case MemoryRequest::STORE_B:
					case MemoryRequest::STORE_THROUGH_B:
						cout << this->name() << " wrote byte " << payload.getPayload() << " to 0x" << std::hex << mActiveAddress << std::dec << endl;
						break;
					default:
					  throw InvalidOptionException("memory store operation", mActiveRequest.getOperation());
						break;
					}
				}

				// Remove the request from the queue after completing it.
				readNextRequest();

				// Chain next request

				if (!processRingEvent())
					processMessageHeader();
			} else {//if (!throughAccess) {                            // temp-removal
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
	assert(mActiveTableIndex < 8 || mChannelMapTable[mActiveTableIndex].Valid);
	assert(mActiveRequest.getOperation() == MemoryRequest::IPK_READ);

	uint core = mActiveTableIndex + id.getTile()*CORES_PER_TILE;
	uint channel = mActiveReturnChannel;

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available

		return;
	} else if (mBankMode == MODE_SCRATCHPAD) {
		assert(mScratchpadModeHandler.containsAddress(mActiveAddress));

		uint32_t data = mScratchpadModeHandler.readWord(mActiveAddress, true, core, channel);
    Instruction inst(data);
    bool endOfPacket = inst.endOfPacket();

    // Enqueue output request

    OutputWord outWord;
    outWord.TableIndex = mActiveTableIndex;
    outWord.ReturnChannel = mActiveReturnChannel;
    outWord.Data = inst;
    outWord.PortClaim = false;
    outWord.LastWord = endOfPacket ||
                       !mScratchpadModeHandler.containsAddress(mActiveAddress+4);

    mOutputQueue.write(outWord);

    // Handle IPK streaming

    if (endOfPacket) {
      if(MEMORY_TRACE == 1) MemoryTrace::stopIPK(cBankNumber, mActiveAddress);

      // Chain next request

      if (!processRingEvent())
        processMessageHeader();
    } else {
      mActiveAddress += 4;

      if (mScratchpadModeHandler.containsAddress(mActiveAddress)) {
        if (!mScratchpadModeHandler.sameLine(mActiveAddress - 4, mActiveAddress))
          if(MEMORY_TRACE == 1) MemoryTrace::splitLineIPK(cBankNumber, mActiveAddress);

        mFSMState = STATE_LOCAL_IPK_READ;
      } else {
        if(MEMORY_TRACE == 1) MemoryTrace::splitBankIPK(cBankNumber, mActiveAddress);

        if (mGroupIndex == mGroupSize - 1) {
          if (mRingRequestOutputPending) {
            mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
            mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
            mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
            mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
            mDelayedRingRequestOutput.Header.PassThrough.Count = 0;
            mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
            mDelayedRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;
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
            mActiveRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;
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
            mDelayedRingRequestOutput.Header.IPKReadHandOff.ReturnChannel = mActiveReturnChannel;
            mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = false;
            mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = 0;

            mFSMState = STATE_WAIT_RING_OUTPUT;
          } else {
            mRingRequestOutputPending = true;
            mActiveRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
            mActiveRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
            mActiveRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
            mActiveRingRequestOutput.Header.IPKReadHandOff.ReturnChannel = mActiveReturnChannel;
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
			cout << this->name() << " reading instruction at address 0x" << std::hex << mActiveAddress << std::dec << " from local cache" << endl;

		assert(mGeneralPurposeCacheHandler.containsAddress(mActiveAddress));

		uint32_t data;
		bool cacheHit = mGeneralPurposeCacheHandler.readWord(mActiveAddress, data, true, mCacheResumeRequest, false, core, channel);

		assert(!(mCacheResumeRequest && !cacheHit));
		mCacheResumeRequest = false;

		if (cacheHit) {

      Instruction inst(data);
      bool endOfPacket = inst.endOfPacket();

      if (DEBUG)
        cout << this->name() << " cache hit " << (endOfPacket ? "(end of packet reached)" : "(end of packet not reached)") << ": " << inst << endl;

      // Enqueue output request

      OutputWord outWord;
      outWord.TableIndex = mActiveTableIndex;
      outWord.ReturnChannel = mActiveReturnChannel;
      outWord.Data = inst;
      outWord.PortClaim = false;
      outWord.LastWord = endOfPacket ||
                         !mGeneralPurposeCacheHandler.containsAddress(mActiveAddress+4);

      mOutputQueue.write(outWord);

      // Handle IPK streaming

      if (endOfPacket) {
        if(MEMORY_TRACE == 1) MemoryTrace::stopIPK(cBankNumber, mActiveAddress);

        // Chain next request

        if (!processRingEvent())
          processMessageHeader();
      } else {
        mActiveAddress += 4;

        if (mGeneralPurposeCacheHandler.containsAddress(mActiveAddress)) {
          if (!mGeneralPurposeCacheHandler.sameLine(mActiveAddress - 4, mActiveAddress))
            if(MEMORY_TRACE == 1) MemoryTrace::splitLineIPK(cBankNumber, mActiveAddress);

          mFSMState = STATE_LOCAL_IPK_READ;
        } else {
          if(MEMORY_TRACE == 1) MemoryTrace::splitBankIPK(cBankNumber, mActiveAddress);

          if (mGroupIndex == mGroupSize - 1) {
            if (mRingRequestOutputPending) {
              mDelayedRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
              mDelayedRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
              mDelayedRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
              mDelayedRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
              mDelayedRingRequestOutput.Header.PassThrough.Count = 0;
              mDelayedRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
              mDelayedRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;
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
              mActiveRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;
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
              mDelayedRingRequestOutput.Header.IPKReadHandOff.ReturnChannel = mActiveReturnChannel;
              mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionPending = false;
              mDelayedRingRequestOutput.Header.IPKReadHandOff.PartialInstructionData = 0;

              mFSMState = STATE_WAIT_RING_OUTPUT;
            } else {
              mRingRequestOutputPending = true;
              mActiveRingRequestOutput.Header.RequestType = RING_IPK_READ_HAND_OFF;
              mActiveRingRequestOutput.Header.IPKReadHandOff.Address = mActiveAddress;
              mActiveRingRequestOutput.Header.IPKReadHandOff.TableIndex = mActiveTableIndex;
              mActiveRingRequestOutput.Header.IPKReadHandOff.ReturnChannel = mActiveReturnChannel;
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
				cout << this->name() << " cache miss" << endl;

			mFSMCallbackState = STATE_LOCAL_IPK_READ;
			mFSMState = STATE_GP_CACHE_MISS;
			mCacheFSMState = GP_CACHE_STATE_PREPARE;
		}
	}
}

void MemoryBank::processLocalBurstRead() {
	//TODO: Implement burst handling completely
	throw UnsupportedFeatureException("memory burst read");

	assert(mBankMode != MODE_INACTIVE);
	assert(mActiveTableIndex < 8 || mChannelMapTable[mActiveTableIndex].Valid);
	assert(mActiveRequest.getOperation() == MemoryRequest::BURST_READ);

	uint core = mActiveTableIndex + id.getTile()*CORES_PER_TILE;
	uint channel = mActiveReturnChannel;

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available

		return;
	} else if (mBankMode == MODE_SCRATCHPAD) {
		assert(mScratchpadModeHandler.containsAddress(mActiveAddress));

		uint32_t data = mScratchpadModeHandler.readWord(mActiveAddress, false, core, channel);
		bool endOfPacket = mActiveBurstLength == 1;

		// Enqueue output request

		OutputWord outWord;
		outWord.TableIndex = mActiveTableIndex;
    outWord.ReturnChannel = mActiveReturnChannel;
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
						mDelayedRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;

						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
						mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
						mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
						mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
						mActiveRingRequestOutput.Header.PassThrough.Count = mActiveBurstLength;
						mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
						mActiveRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;

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
						mDelayedRingRequestOutput.Header.BurstReadHandOff.ReturnChannel = mActiveReturnChannel;

						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mActiveRingRequestOutput.Header.RequestType = RING_BURST_READ_HAND_OFF;
						mActiveRingRequestOutput.Header.BurstReadHandOff.Address = mActiveAddress;
						mActiveRingRequestOutput.Header.BurstReadHandOff.Count = mActiveBurstLength;
						mActiveRingRequestOutput.Header.BurstReadHandOff.TableIndex = mActiveTableIndex;
						mActiveRingRequestOutput.Header.BurstReadHandOff.ReturnChannel = mActiveReturnChannel;

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
		bool cacheHit = mGeneralPurposeCacheHandler.readWord(mActiveAddress, data, false, mCacheResumeRequest, false, core, channel);

		assert(!(mCacheResumeRequest && !cacheHit));
		mCacheResumeRequest = false;

		if (cacheHit) {
			bool endOfPacket = mActiveBurstLength == 1;

			// Enqueue output request

			OutputWord outWord;
			outWord.TableIndex = mActiveTableIndex;
      outWord.ReturnChannel = mActiveReturnChannel;
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
							mDelayedRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_PASS_THROUGH;
							mActiveRingRequestOutput.Header.PassThrough.DestinationBankNumber = mGroupBaseBank;
							mActiveRingRequestOutput.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.PassThrough.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.PassThrough.Count = mActiveBurstLength;
							mActiveRingRequestOutput.Header.PassThrough.TableIndex = mActiveTableIndex;
							mActiveRingRequestOutput.Header.PassThrough.ReturnChannel = mActiveReturnChannel;

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
							mDelayedRingRequestOutput.Header.BurstReadHandOff.ReturnChannel = mActiveReturnChannel;

							mFSMState = STATE_WAIT_RING_OUTPUT;
						} else {
							mRingRequestOutputPending = true;
							mActiveRingRequestOutput.Header.RequestType = RING_BURST_READ_HAND_OFF;
							mActiveRingRequestOutput.Header.BurstReadHandOff.Address = mActiveAddress;
							mActiveRingRequestOutput.Header.BurstReadHandOff.Count = mActiveBurstLength;
							mActiveRingRequestOutput.Header.BurstReadHandOff.TableIndex = mActiveTableIndex;
							mActiveRingRequestOutput.Header.BurstReadHandOff.ReturnChannel = mActiveReturnChannel;

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
	throw UnsupportedFeatureException("memory burst write");
}

void MemoryBank::processBurstWriteSlave() {
	//TODO: Implement handler
  throw UnsupportedFeatureException("memory burst write");
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
		throw InvalidOptionException("cache state", mCacheFSMState);
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

void MemoryBank::processValidRing() {
	// Acknowledge new data as soon as it arrives if we know it will be consumed
	// in this clock cycle.
  bool ack = iRingStrobe.read() && !mRingRequestInputPending;
	if(ack != oRingAcknowledge.read()) oRingAcknowledge.write(ack);
}

void MemoryBank::processValidInput() {
  if(DEBUG) cout << this->name() << " received " << iData.read() << endl;
  assert(iData.valid());
  assert(!mInputQueue.full());

  mInputQueue.write(iData.read());
  iData.ack();
}

void MemoryBank::handleDataOutput() {
  // If we have new data to send:
  if(!mOutputWordPending) {
    if(mOutputQueue.empty()) {
      next_trigger(mOutputQueue.writeEvent());
    }
    else {
      OutputWord outWord = mOutputQueue.read();

      // For 0-7, tile=tile, core=channel, channel=part of request
      // For 8-15, return address is held in channel map table
      ChannelID returnAddr;
      if (outWord.TableIndex < 8) {
        returnAddr = ChannelID(id.getTile(), outWord.TableIndex, outWord.ReturnChannel);
      }
      else {
        returnAddr = mChannelMapTable[outWord.TableIndex].ReturnChannel;
      }
      NetworkData word(outWord.Data, returnAddr);
      word.setPortClaim(outWord.PortClaim, false);
      word.setEndOfPacket(outWord.LastWord);
      mOutputWordPending = true;

      // Remove the previous request if we are sending to a new destination.
      // Is this a hack, or is this reasonable?
      if(mActiveOutputWord.channelID().getComponentID() != word.channelID().getComponentID())
        localNetwork->makeRequest(id, mActiveOutputWord.channelID(), false);

      mActiveOutputWord = word;

      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), true);
      next_trigger(iClock.negedge_event());
    }
  }
  // Check to see whether we are allowed to send data yet.
  else if(!localNetwork->requestGranted(id, mActiveOutputWord.channelID())) {
    // If not, wait another cycle.
    next_trigger(iClock.negedge_event());
  }
  // If it is time to send data:
  else {

    if (DEBUG)
      cout << this->name() << " sent " << mActiveOutputWord << endl;
    if (ENERGY_TRACE)
      Instrumentation::Network::traffic(id, mActiveOutputWord.channelID().getComponentID());

    oData.write(mActiveOutputWord);

    // Remove the request for network resources.
    if(mActiveOutputWord.endOfPacket())
      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), false);

    mOutputWordPending = false;

    next_trigger(oData.ack_event());
  }
}

// Method called every time we see an event on the iRequest port.
void MemoryBank::handleRequestInput() {
  if (DEBUG)
    cout << this->name() << " received request " << iRequest.read() << endl;

  assert(iRequest.valid());
  assert(!mInputReqQueue.full());
  mInputReqQueue.write(iRequest.read());
  iRequest.ack();
}

// Method which sends data from the mOutputReqQueue whenever possible.
void MemoryBank::handleRequestOutput() {
  if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else if (mOutputReqQueue.empty())
    next_trigger(mOutputReqQueue.writeEvent());
  else {
    assert(!oRequest.valid());
    oRequest.write(mOutputReqQueue.read());
    next_trigger(oRequest.ack_event());

    if (DEBUG)
      cout << this->name() << " sent request " << oRequest.read() << endl;
  }
}

// Method called every time we see an event on the iResponse port.
void MemoryBank::handleResponseInput() {
  if (DEBUG)
    cout << this->name() << " received response " << iResponse.read() << endl;

  assert(iResponse.valid());
  assert(!mInputRespQueue.full());
  mInputRespQueue.write(iResponse.read());
  iResponse.ack();
}

// Method which sends data from the mOutputRespQueue whenever possible.
void MemoryBank::handleResponseOutput() {
  if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else if (mOutputRespQueue.empty())
    next_trigger(mOutputRespQueue.writeEvent());
  else {
    assert(!oResponse.valid());
    oResponse.write(mOutputRespQueue.read());
    next_trigger(oResponse.ack_event());

    if (DEBUG)
      cout << this->name() << " sent response " << oResponse.read() << endl;
  }
}

void MemoryBank::handleNetworkInterfacesPre() {

  // Set output port defaults - only final write will be effective
  if(oRingStrobe.read())   oRingStrobe.write(false);
  if(oBMDataStrobe.read()) oBMDataStrobe.write(false);

  // Check whether old ring output got acknowledged

  if (mRingRequestOutputPending && iRingAcknowledge.read()) {
    if (DEBUG)
      cout << this->name() << " ring request got acknowledged" << endl;

    mRingRequestOutputPending = false;
  }

  // Pass the memory operation on to the next bank, if possible.
  // To ensure that everything arrives in the correct order, all data must have
  // been sent before the request can be passed on.

  if (mRingRequestOutputPending && !mOutputWordPending && mOutputQueue.empty()) {
    oRingStrobe.write(true);
    oRingRequest.write(mActiveRingRequestOutput);
  }

}

void MemoryBank::mainLoop() {

  if(iClock.posedge()) {

    handleNetworkInterfacesPre();

  }
  else if(iClock.negedge()) {

    // Check ring input ports and update request registers

    if (iRingStrobe.read() && !mRingRequestInputPending) {
      // Acknowledgement handled by separate methods

      mRingRequestInputPending = true;
      mActiveRingRequestInput = iRingRequest.read();
    }

    // Proceed according to FSM state

    // If we have a "fast" memory, decode the request and get into the correct
    // state before performing the operation.
    // Also, don't allow a new operation to start until any ring requests have
    // been passed on - ensures that data is sent in correct order.
    if(FAST_MEMORY && mFSMState == STATE_IDLE && !mRingRequestOutputPending) {
      if (!processRingEvent())
        processMessageHeader();
    }

    switch (mFSMState) {
    case STATE_IDLE:
      // If the memory is not "fast", the operation has to be decoded in one
      // cycle, and performed in the next.
      // Also, don't allow a new operation to start until any ring requests have
      // been passed on - ensures that data is sent in correct order.
      if(!FAST_MEMORY && !mRingRequestOutputPending) {
        if (!processRingEvent())
          processMessageHeader();
      }

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
      throw InvalidOptionException("memory bank state", mFSMState);
      break;
    }
  }

  // Update status signals

  bool wasIdle = currentlyIdle;
  currentlyIdle = mFSMState == STATE_IDLE &&
                  mInputQueue.empty() && mOutputQueue.empty() &&
                  mInputReqQueue.empty() && mOutputReqQueue.empty() &&
                  !mOutputWordPending &&
                  !mRingRequestInputPending && !mRingRequestOutputPending;

  if (wasIdle != currentlyIdle) {
    Instrumentation::idle(id, currentlyIdle);
  }

  bool ready = !mInputQueue.full();
  if (ready != oReadyForData.read())
    oReadyForData.write(ready);
  bool ready2 = !mInputReqQueue.full();
  if (ready2 != oReadyForRequest.read())
    oReadyForRequest.write(ready2);
}

//-------------------------------------------------------------------------------------------------
// Constructors and destructors
//-------------------------------------------------------------------------------------------------

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber) :
	Component(name, ID),
  mInputQueue(IN_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mInputQueue")),
  mOutputQueue(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mOutputQueue")),
  mInputReqQueue(IN_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mInputReqQueue")),
  mOutputReqQueue(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mOutputReqQueue")),
  mInputRespQueue(IN_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mInputRespQueue")),
  mOutputRespQueue(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mOutputRespQueue")),
	mScratchpadModeHandler(bankNumber),
	mGeneralPurposeCacheHandler(bankNumber)
{
	//-- Configuration parameters -----------------------------------------------------------------

	cChannelMapTableEntries = MEMORY_CHANNEL_MAP_TABLE_ENTRIES;
	cMemoryBanks = MEMS_PER_TILE;
	cBankNumber = bankNumber;
	mWayCount = 0;
	mLineSize = 0;
	cRandomReplacement = MEMORY_CACHE_RANDOM_REPLACEMENT != 0;

	assert(cChannelMapTableEntries > 0);
	assert(cMemoryBanks > 0);
	assert(cBankNumber < cMemoryBanks);

	//-- Data queue state -------------------------------------------------------------------------

	mOutputWordPending = false;
	mCurrentInput = INPUT_NONE;

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

	oReadyForData.initialize(false);
	oReadyForRequest.initialize(false);
	oBMDataStrobe.initialize(false);
	oRingAcknowledge.initialize(false);
	oRingStrobe.initialize(false);

	//-- Register module with SystemC simulation kernel -------------------------------------------

	currentlyIdle = true;
	Instrumentation::idle(id, true);

	SC_METHOD(mainLoop);
	sensitive << iClock;
	dont_initialize();

	SC_METHOD(processValidRing);
	sensitive << iRingStrobe << iClock.neg();
	// do initialise

	SC_METHOD(processValidInput);
	sensitive << iData;
	dont_initialize();

	SC_METHOD(handleDataOutput);

  SC_METHOD(handleRequestInput);
  sensitive << iRequest;
  dont_initialize();

  SC_METHOD(handleRequestOutput);

  SC_METHOD(handleResponseInput);
  sensitive << iResponse;
  dont_initialize();

  SC_METHOD(handleResponseOutput);

	end_module(); // Needed because we're using a different Component constructor
}

MemoryBank::~MemoryBank() {
	delete[] mChannelMapTable;
}

void MemoryBank::setAdjacentMemories(MemoryBank *prevMemoryBank, MemoryBank *nextMemoryBank, SimplifiedOnChipScratchpad *backgroundMemory) {
	mPrevMemoryBank = prevMemoryBank;
	mNextMemoryBank = nextMemoryBank;
	mBackgroundMemory = backgroundMemory;
	mGeneralPurposeCacheHandler.setBackgroundMemory(backgroundMemory);
}

void MemoryBank::setLocalNetwork(local_net_t* network) {
  localNetwork = network;
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	size_t count = data.size();
	uint32_t address = location;

	// Don't want to record stats for this access
	uint core = -1;
	uint channel = -1;

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
					bank->mScratchpadModeHandler.writeWord(address + i * 4, data[i].toUInt(), core, channel);
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
					if (!bank->mGeneralPurposeCacheHandler.writeWord(address + i * 4, data[i].toUInt(), false, true, core, channel))
						mBackgroundMemory->writeWord(address + i * 4, data[i]);

					break;
				}

				bank = bank->mNextMemoryBank;
			}
		}
	}
}

void MemoryBank::synchronizeData() {
	assert(mBackgroundMemory != 0);
	if (mBankMode == MODE_GP_CACHE)
		mGeneralPurposeCacheHandler.synchronizeData(mBackgroundMemory);
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

  // Don't want to record stats for this access
  uint core = -1;
  uint channel = -1;

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
					uint32_t data = bank->mScratchpadModeHandler.readWord(address, false, core, channel);
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
					bool cached = bank->mGeneralPurposeCacheHandler.readWord(address, data, false, false, true, core, channel);

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

  // Don't want to record stats for this access
  uint core = -1;
  uint channel = -1;

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr))
				return Word(bank->mScratchpadModeHandler.readWord(addr, false, core, channel));

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return Word(0);
	} else {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mGeneralPurposeCacheHandler.containsAddress(addr)) {
				uint32_t data;
				bool cached = bank->mGeneralPurposeCacheHandler.readWord(addr, data, false, false, true, core, channel);

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

  // Don't want to record stats for this access
  uint core = -1;
  uint channel = -1;

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr))
				return Word(bank->mScratchpadModeHandler.readByte(addr, core, channel));

			bank = bank->mNextMemoryBank;
		}

		assert(false);
		return Word(0);
	} else {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mGeneralPurposeCacheHandler.containsAddress(addr)) {
				uint32_t data;
				bool cached = bank->mGeneralPurposeCacheHandler.readByte(addr, data, false, true, core, channel);

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

  // Don't want to record stats for this access
  uint core = -1;
  uint channel = -1;

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr)) {
				bank->mScratchpadModeHandler.writeWord(addr, data.toUInt(), core, channel);
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
				bool cached = bank->mGeneralPurposeCacheHandler.writeWord(addr, data.toUInt(), false, true, core, channel);

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

  // Don't want to record stats for this access
  uint core = -1;
  uint channel = -1;

	MemoryBank *firstBank = this;
	while (firstBank->mGroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mBankMode == MODE_SCRATCHPAD) {
		MemoryBank *bank = firstBank;

		for (uint groupIndex = 0; groupIndex < mGroupSize; groupIndex++) {
			if (bank->mScratchpadModeHandler.containsAddress(addr)) {
				bank->mScratchpadModeHandler.writeByte(addr, data.toUInt(), core, channel);
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
				bool cached = bank->mGeneralPurposeCacheHandler.writeByte(addr, data.toUInt(), false, true, core, channel);

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

void MemoryBank::reportStalls(ostream& os) {
  if (mInputQueue.full()) {
    os << mInputQueue.name() << " is full." << endl;
  }
  if (!mOutputQueue.empty()) {
    const OutputWord& outWord = mOutputQueue.peek();
    ChannelID addr;
    if (outWord.TableIndex < 8)
      addr = ChannelID(id.getTile(), outWord.TableIndex, outWord.ReturnChannel);
    else
      addr = mChannelMapTable[outWord.TableIndex].ReturnChannel;

    os << this->name() << " waiting to send to " << addr << endl;
  }
}
