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

#include "MemoryBank.h"
#include "../../Datatype/Instruction.h"
#include "../../Network/Topologies/LocalNetwork.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../../Utility/Trace/MemoryTrace.h"
#include "../../Utility/Parameters.h"
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

struct MemoryBank::RingNetworkRequest& MemoryBank::getAvailableRingRequest() {
  return mRingRequestOutputPending ? mDelayedRingRequestOutput : mActiveRingRequestOutput;
}

void MemoryBank::updatedRingRequest() {
  if (mRingRequestOutputPending)
    mFSMState = STATE_WAIT_RING_OUTPUT;
  else {
    mRingRequestOutputPending = true;
    mFSMState = STATE_IDLE;
  }
}

AbstractMemoryHandler& MemoryBank::currentMemoryHandler() {
  switch (mConfig.Mode) {
    case MODE_SCRATCHPAD:
      return mScratchpadModeHandler;
    case MODE_GP_CACHE:
      return mGeneralPurposeCacheHandler;
    default:
      throw InvalidOptionException("current memory handler", mConfig.Mode);
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

		mConfig = mActiveRingRequestInput.Header.SetMode;

		assert(mConfig.GroupIndex <= mConfig.GroupSize);

		currentMemoryHandler().activate(mConfig);

		if (mConfig.GroupIndex < mConfig.GroupSize - 1) {
			// Forward request through ring network

      RingNetworkRequest& request = getAvailableRingRequest();

      request.Header.RequestType = RING_SET_MODE;
      request.Header.SetMode = mConfig;
      request.Header.SetMode.GroupIndex++;

      updatedRingRequest();

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

		mActiveData.TableIndex = mActiveRingRequestInput.Header.SetTableEntry.TableIndex;
		mChannelMapTable[mActiveData.TableIndex].Valid = true;
		mChannelMapTable[mActiveData.TableIndex].ReturnChannel = mActiveRingRequestInput.Header.SetTableEntry.TableEntry;

		if (mConfig.GroupIndex < mConfig.GroupSize - 1) {
			// Forward request through ring network

      RingNetworkRequest& request = getAvailableRingRequest();

      request.Header.RequestType = RING_SET_TABLE_ENTRY;
      request.Header.SetTableEntry.TableIndex = mActiveData.TableIndex;
      request.Header.SetTableEntry.TableEntry = mActiveRingRequestInput.Header.SetTableEntry.TableEntry;

      updatedRingRequest();
		} else {
			// Send port claim message to return channel
			OutputWord outWord(ChannelID(id, mActiveData.TableIndex).flatten(),
			    mActiveData.TableIndex, 0, true, true);
			mOutputQueue.write(outWord);

			mFSMState = STATE_IDLE;
		}

		break;

	case RING_BURST_READ_HAND_OFF:
		if (DEBUG)
			cout << this->name() << " received RING_BURST_READ_HAND_OFF request through ring network" << endl;

		// A bit of a hack but simplifies the code considerably

		mActiveRequest = MemoryRequest(MemoryRequest::BURST_READ, 0);
		mActiveData    = mActiveRingRequestInput.Header.Request;

		mFSMState = STATE_LOCAL_BURST_READ;
		break;

	case RING_BURST_WRITE_FORWARD:
		if (DEBUG)
			cout << this->name() << " received RING_BURST_WRITE_FORWARD request through ring network" << endl;

		// A bit of a hack but simplifies the code considerably

		mActiveRequest = MemoryRequest(MemoryRequest::BURST_WRITE, 0);
    mActiveData    = mActiveRingRequestInput.Header.Request;

		mFSMState = STATE_BURST_WRITE_SLAVE;
		break;

	case RING_IPK_READ_HAND_OFF:
		if (DEBUG)
			cout << this->name() << " received RING_IPK_READ_HAND_OFF request through ring network" << endl;

		// A bit of a hack but simplifies the code considerably

		mActiveRequest = MemoryRequest(MemoryRequest::IPK_READ, 0);
    mActiveData    = mActiveRingRequestInput.Header.Request;

		if (ENERGY_TRACE)
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
			case RING_IPK_READ_HAND_OFF:
			  mActiveRingRequestInput.Header.Request = request.Header.PassThrough.Request;
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

  NetworkData request = mInputQueue.peek();
	mActiveData.TableIndex = request.channelID().channel;
	mActiveData.ReturnChannel = request.returnAddr();
	mActiveRequest = MemoryRequest(request.payload());
	bool inputWordProcessed = true;

	switch (mActiveRequest.getOperation()) {
	case MemoryRequest::CONTROL:
		if (DEBUG)
			cout << this->name() << " received CONTROL request on channel " << mActiveData.TableIndex << endl;

		if (mChannelMapTable[mActiveData.TableIndex].FetchPending) {
			if (DEBUG)
				cout << this->name() << " received channel address" << endl;

			if (!mOutputQueue.empty()) {
			  if (DEBUG)
			    cout << this->name() << " output queue not empty" << endl;

				// Drain output queue before changing table

				inputWordProcessed = false;
				mFSMState = STATE_IDLE;
			} else {
				// Update channel map table

				mChannelMapTable[mActiveData.TableIndex].Valid = true;
				mChannelMapTable[mActiveData.TableIndex].FetchPending = false;
				mChannelMapTable[mActiveData.TableIndex].ReturnChannel = mActiveRequest.getChannelID();

				if (mConfig.GroupIndex < mConfig.GroupSize - 1) {
					// Forward request through ring network

          RingNetworkRequest& request =
              mRingRequestOutputPending ? mDelayedRingRequestOutput : mActiveRingRequestOutput;

          if (mRingRequestOutputPending) {
						mFSMState = STATE_WAIT_RING_OUTPUT;
					} else {
						mRingRequestOutputPending = true;
						mFSMState = STATE_IDLE;
					}

					request.Header.RequestType = RING_SET_TABLE_ENTRY;
          request.Header.SetTableEntry.TableIndex = mActiveData.TableIndex;
          request.Header.SetTableEntry.TableEntry = mActiveRequest.getChannelID();

				} else {
					// Send port claim message to return channel
					OutputWord outWord(ChannelID(id, mActiveData.TableIndex).flatten(),
					    mActiveData.TableIndex, 0, true, true);
					mOutputQueue.write(outWord);

					mFSMState = STATE_IDLE;
				}
			}
		} else if (mActiveRequest.getOpCode() == MemoryRequest::SET_MODE) {
			if (DEBUG)
				cout << this->name() << " received SET_MODE opcode on channel " << mActiveData.TableIndex << endl;

			uint wayBits = mActiveRequest.getWayBits();
			uint lineBits = mActiveRequest.getLineBits();

			mConfig.WayCount = 1U << wayBits;
			mConfig.LineSize = 1U << lineBits;

			mConfig.GroupBaseBank = cBankNumber;
			mConfig.GroupIndex = 0;
			mConfig.GroupSize = 1UL << mActiveRequest.getGroupBits();

			assert(mConfig.GroupIndex <= mConfig.GroupSize);

			if (mActiveRequest.getMode() == MemoryRequest::SCRATCHPAD) {
				mConfig.Mode = MODE_SCRATCHPAD;
				mScratchpadModeHandler.activate(mConfig);
			} else {
				mConfig.Mode = MODE_GP_CACHE;
				mGeneralPurposeCacheHandler.activate(mConfig);
			}

			// Print a description of the configuration, so we can more-easily tell
			// whether cores are acting consistently.
			if (DEBUG || Arguments::summarise())
			  printConfiguration();

			if (mConfig.GroupIndex < mConfig.GroupSize - 1) {
				// Forward request through ring network

        RingNetworkRequest& request = getAvailableRingRequest();

        request.Header.RequestType = RING_SET_MODE;
        request.Header.SetMode = mConfig;
        request.Header.SetMode.GroupIndex++;

        updatedRingRequest();

			} else {
				mFSMState = STATE_IDLE;
			}
		} else {
			assert(mActiveRequest.getOpCode() == MemoryRequest::SET_CHMAP);

			if (DEBUG)
				cout << this->name() << " received SET_CHMAP opcode on channel " << mActiveData.TableIndex << endl;

			mChannelMapTable[mActiveData.TableIndex].FetchPending = true;

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
			cout << this->name() << " received scalar request on channel " << mActiveData.TableIndex << endl;

		mActiveData.Address = mActiveRequest.getPayload();
		mFSMState = STATE_LOCAL_MEMORY_ACCESS;

		break;

	case MemoryRequest::IPK_READ:
		if (DEBUG)
			cout << this->name() << " received IPK_READ request on channel " << mActiveData.TableIndex << endl;

		if (ENERGY_TRACE)
		  Instrumentation::memoryInitiateIPKRead(cBankNumber, false);

		mActiveData.Address = mActiveRequest.getPayload();
		mFSMState = STATE_LOCAL_IPK_READ;

		break;

	case MemoryRequest::BURST_READ:
		if (DEBUG)
			cout << this->name() << " received BURST_READ request on channel " << mActiveData.TableIndex << endl;

		mActiveData.Address = mActiveRequest.getPayload();
		mFSMCallbackState = STATE_LOCAL_BURST_READ;
		mFSMState = STATE_FETCH_BURST_LENGTH;

		break;

	case MemoryRequest::BURST_WRITE:
		if (DEBUG)
			cout << this->name() << " received BURST_WRITE request on channel " << mActiveData.TableIndex << endl;

		mActiveData.Address = mActiveRequest.getPayload();
		mFSMCallbackState = STATE_BURST_WRITE_MASTER;
		mFSMState = STATE_FETCH_BURST_LENGTH;

		break;

	case MemoryRequest::DIRECTORY_UPDATE:
    if (DEBUG)
      cout << this->name() << " received DIRECTORY_UPDATE request on channel " << mActiveData.TableIndex << endl;

    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(mActiveRequest);
    mFSMState = STATE_IDLE;
    break;

	case MemoryRequest::DIRECTORY_MASK_UPDATE:
    if (DEBUG)
      cout << this->name() << " received DIRECTORY_MASK_UPDATE request on channel " << mActiveData.TableIndex << endl;

    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(mActiveRequest);
    mFSMState = STATE_IDLE;
	  break;

	default:
		cout << this->name() << " received invalid memory request type (" << mActiveRequest.getOperation() << ")" << endl;
		assert(false);
		break;
	}

	if (inputWordProcessed) {
	  mInputQueue.read();  // We'd already peeked at the request - now discard it.
		return true;
	} else {
		return false;
	}
}

void MemoryBank::processFetchBurstLength() {
	if (!mInputQueue.empty()) {
		MemoryRequest request(mInputQueue.read().payload());
		assert(request.getOperation() == MemoryRequest::PAYLOAD_ONLY);
		mActiveData.Count = request.getPayload();
		mFSMState = mFSMCallbackState;
	}
}

void MemoryBank::processLocalMemoryAccess() {
	assert(mConfig.Mode != MODE_INACTIVE);
	assert(mActiveData.TableIndex < 8 || mChannelMapTable[mActiveData.TableIndex].Valid);

	if (mOutputQueue.full() && (mActiveRequest.isSingleLoad())) {
		// Delay load request until there is room in the output queue available

		if (DEBUG)
			cout << this->name() << " delayed scalar request due to full output queue" << endl;

		return;
	} else {
	  AbstractMemoryHandler& handler = currentMemoryHandler();
		assert(handler.containsAddress(mActiveData.Address));

		if (mActiveRequest.isSingleLoad()) {
			bool cacheHit;
			uint32_t data;

			switch (mActiveRequest.getOperation()) {
			case MemoryRequest::LOAD_W:		cacheHit = handler.readWord(mActiveData.Address, data, false, mCacheResumeRequest, false);	break;
			case MemoryRequest::LOAD_HW:	cacheHit = handler.readHalfWord(mActiveData.Address, data, mCacheResumeRequest, false);		break;
			case MemoryRequest::LOAD_B:		cacheHit = handler.readByte(mActiveData.Address, data, mCacheResumeRequest, false);			break;

			// FIXME: temporary hack. Desired code is commented out below.
			case MemoryRequest::LOAD_THROUGH_W:
        cacheHit = true;
        data = mBackgroundMemory->readWord(mActiveData.Address).toUInt();
        mActiveRequest.clearThroughAccess();
        break;
      case MemoryRequest::LOAD_THROUGH_HW:
        cacheHit = true;
        data = mBackgroundMemory->readWord(mActiveData.Address & ~3).toUInt();
        if (mActiveData.Address & 2)
          data = (data >> 16) & 0xFFFF;
        else
          data = data & 0xFFFF;
        mActiveRequest.clearThroughAccess();
        break;
      case MemoryRequest::LOAD_THROUGH_B:
        cacheHit = true;
        data = mBackgroundMemory->readByte(mActiveData.Address).toUInt();
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
			default:						assert(false);																								break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			if (cacheHit) {
				if (DEBUG)
				  printOperation(mActiveRequest.getOperation(), mActiveData.Address, data);

				// Enqueue output request
				OutputWord outWord(Word(data), mActiveData.TableIndex, mActiveData.ReturnChannel, false, true);
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
			if (mInputQueue.empty())
				return;

			bool cacheHit;
			MemoryRequest payload(mInputQueue.peek().payload());

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
				cout << "!!!! " << mActiveRequest.getOperation() << " | " << mActiveData.Address << " | " << payload.getOperation() << " | " << payload.getPayload() << endl;
			}

			// The received word should be the continuation of the previous operation
			// (and should come from the same source core?).
//			assert(mActiveRequest.getChannelID().getComponentID().getPosition() == payload.getChannelID().getComponentID().getPosition());
			assert(payload.getOperation() == MemoryRequest::PAYLOAD_ONLY);

			switch (mActiveRequest.getOperation()) {
//			case MemoryRequest::STORE_THROUGH_W:
			case MemoryRequest::STORE_W:	cacheHit = handler.writeWord(mActiveData.Address, payload.getPayload(), mCacheResumeRequest, false);		break;
//			case MemoryRequest::STORE_THROUGH_HW:
			case MemoryRequest::STORE_HW:	cacheHit = handler.writeHalfWord(mActiveData.Address, payload.getPayload(), mCacheResumeRequest, false);	break;
//			case MemoryRequest::STORE_THROUGH_B:
			case MemoryRequest::STORE_B:	cacheHit = handler.writeByte(mActiveData.Address, payload.getPayload(), mCacheResumeRequest, false);		break;

			// FIXME: temporary hack. Would prefer to use commented out code both
			// above and below.
      case MemoryRequest::STORE_THROUGH_W:
        cacheHit = true;
        mBackgroundMemory->writeWord(mActiveData.Address, payload.getPayload());
        break;
      case MemoryRequest::STORE_THROUGH_HW: {
        cacheHit = true;
        uint32_t olddata = mBackgroundMemory->readWord(mActiveData.Address & ~3).toUInt();
        uint32_t newdata;
        if (mActiveData.Address & 2)
          newdata = ((payload.getPayload() & 0xFFFF) << 16) | (olddata & 0xFFFF);
        else
          newdata = ((olddata & 0xFFFF0000)) | (payload.getPayload() & 0xFFFF);

        mBackgroundMemory->writeWord(mActiveData.Address & ~3, newdata);
        break;
      }
      case MemoryRequest::STORE_THROUGH_B:
        cacheHit = true;
        mBackgroundMemory->writeByte(mActiveData.Address, payload.getPayload());
        break;

			default:						assert(false);																											break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			// If this is a through access, we have to write the word back to the
			// next level of cache.
			//cacheHit = cacheHit && !throughAccess;

			if (cacheHit) {
				if (DEBUG)
				  printOperation(mActiveRequest.getOperation(), mActiveData.Address, payload.getPayload());

				// Remove the request from the queue after completing it.
				mInputQueue.read();

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
	assert(mConfig.Mode != MODE_INACTIVE);
	assert(mActiveData.TableIndex < 8 || mChannelMapTable[mActiveData.TableIndex].Valid);
	assert(mActiveRequest.getOperation() == MemoryRequest::IPK_READ);

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available
		return;
	} else {
		if (DEBUG)
			cout << this->name() << " reading instruction at address 0x" << std::hex << mActiveData.Address << std::dec << " from local cache" << endl;

		AbstractMemoryHandler& handler = currentMemoryHandler();

		assert(handler.containsAddress(mActiveData.Address));

		uint32_t data;
		bool cacheHit = handler.readWord(mActiveData.Address, data, true, mCacheResumeRequest, false);

		assert(!(mCacheResumeRequest && !cacheHit));
		mCacheResumeRequest = false;

		if (cacheHit) {
		  prepareIPKReadOutput(handler, data);
		} else {
			if (DEBUG)
				cout << this->name() << " cache miss" << endl;

			mFSMCallbackState = STATE_LOCAL_IPK_READ;
			mFSMState = STATE_GP_CACHE_MISS;
			mCacheFSMState = GP_CACHE_STATE_PREPARE;
		}
	}
}

void MemoryBank::prepareIPKReadOutput(AbstractMemoryHandler& handler, uint32_t data) {
  Instruction inst(data);
  bool endOfPacket = inst.endOfPacket();

  if (DEBUG)
    cout << this->name() << " cache hit " << (endOfPacket ? "(end of packet reached)" : "(end of packet not reached)") << ": " << inst << endl;

  // Enqueue output request
  OutputWord outWord(inst, mActiveData.TableIndex, mActiveData.ReturnChannel, false,
                     endOfPacket || !handler.containsAddress(mActiveData.Address+4));
  mOutputQueue.write(outWord);

  // Handle IPK streaming

  if (endOfPacket) {
    if (MEMORY_TRACE == 1) MemoryTrace::stopIPK(cBankNumber, mActiveData.Address);

    // Chain next request
    if (!processRingEvent())
      processMessageHeader();
  } else {
    mActiveData.Address += 4;

    if (handler.containsAddress(mActiveData.Address)) {
      if (!handler.sameLine(mActiveData.Address - 4, mActiveData.Address))
        if (MEMORY_TRACE == 1) MemoryTrace::splitLineIPK(cBankNumber, mActiveData.Address);

      mFSMState = STATE_LOCAL_IPK_READ;
    } else {
      if (MEMORY_TRACE == 1)
        MemoryTrace::splitBankIPK(cBankNumber, mActiveData.Address);

      RingNetworkRequest& request = getAvailableRingRequest();

      if (mConfig.GroupIndex == mConfig.GroupSize - 1) {
        request.Header.RequestType = RING_PASS_THROUGH;
        request.Header.PassThrough.DestinationBankNumber = mConfig.GroupBaseBank;
        request.Header.PassThrough.EnvelopedRequestType = RING_IPK_READ_HAND_OFF;
        request.Header.PassThrough.Request = mActiveData;
        request.Header.PassThrough.Request.Count = 0;
        request.Header.PassThrough.Request.PartialInstructionPending = false;
        request.Header.PassThrough.Request.PartialInstructionData = 0;
      } else {
        request.Header.RequestType = RING_IPK_READ_HAND_OFF;
        request.Header.Request = mActiveData;
        request.Header.Request.PartialInstructionPending = false;
        request.Header.Request.PartialInstructionData = 0;
      }

      bool chainRequest = !mRingRequestOutputPending && !processRingEvent();
      updatedRingRequest();
      if (chainRequest)
        processMessageHeader();
    }
  }
}

void MemoryBank::processLocalBurstRead() {
	//TODO: Implement burst handling completely
	assert(false);

	assert(mConfig.Mode != MODE_INACTIVE);
	assert(mActiveData.TableIndex < 8 || mChannelMapTable[mActiveData.TableIndex].Valid);
	assert(mActiveRequest.getOperation() == MemoryRequest::BURST_READ);

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available

		return;
	} else {
	  AbstractMemoryHandler& handler = currentMemoryHandler();
		assert(handler.containsAddress(mActiveData.Address));

		uint32_t data;
		bool cacheHit = handler.readWord(mActiveData.Address, data, false, mCacheResumeRequest, false);

		assert(!(mCacheResumeRequest && !cacheHit));
		mCacheResumeRequest = false;

		if (cacheHit) {
		  prepareBurstReadOutput(handler, data);
		} else {
			mFSMCallbackState = STATE_LOCAL_BURST_READ;
			mFSMState = STATE_GP_CACHE_MISS;
			mCacheFSMState = GP_CACHE_STATE_PREPARE;
		}
	}
}

void MemoryBank::prepareBurstReadOutput(AbstractMemoryHandler& handler, uint32_t data) {
  bool endOfPacket = mActiveData.Count == 1;

  // Enqueue output request
  OutputWord outWord(data, mActiveData.TableIndex, mActiveData.ReturnChannel, false, endOfPacket);
  mOutputQueue.write(outWord);

  // Handle data streaming

  if (endOfPacket) {
    // Chain next request

    if (!processRingEvent())
      processMessageHeader();
  } else {
    mActiveData.Address += 4;
    mActiveData.Count--;

    if (handler.containsAddress(mActiveData.Address)) {
      mFSMState = STATE_LOCAL_BURST_READ;
    } else {
      RingNetworkRequest& request = getAvailableRingRequest();

      if (mConfig.GroupIndex == mConfig.GroupSize - 1) {
        request.Header.RequestType = RING_PASS_THROUGH;
        request.Header.PassThrough.DestinationBankNumber = mConfig.GroupBaseBank;
        request.Header.PassThrough.EnvelopedRequestType = RING_BURST_READ_HAND_OFF;
        request.Header.PassThrough.Request = mActiveData;
      } else {
        request.Header.RequestType = RING_BURST_READ_HAND_OFF;
        request.Header.Request = mActiveData;
      }

      bool chainRequest = !mRingRequestOutputPending && !processRingEvent();
      updatedRingRequest();
      if (chainRequest)
        processMessageHeader();
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
	case GP_CACHE_STATE_PREPARE: {
		mGeneralPurposeCacheHandler.prepareCacheLine(mActiveData.Address, mWriteBackAddress, mWriteBackCount, mCacheLineBuffer, mFetchAddress, mFetchCount);
		mCacheLineCursor = 0;

    // We only start a memory operation if we know there is buffer space to
    // complete it.
    assert(!mOutputReqQueue.full());

		if (mWriteBackCount > 0) {
			assert(mWriteBackCount == mConfig.LineSize / 4);

			MemoryRequest header(MemoryRequest::STORE_LINE, mWriteBackAddress, mConfig.LineSize/4, id.tile, id.position);
			mOutputReqQueue.write(header);

			mCacheFSMState = GP_CACHE_STATE_SEND_DATA;
		} else {
			assert(mFetchCount == mConfig.LineSize / 4);

			if (DEBUG)
			  cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes from 0x" << std::hex << mFetchAddress << std::dec << endl;

			MemoryRequest header(MemoryRequest::FETCH_LINE, mFetchAddress, mConfig.LineSize/4, id.tile, id.position);
			mOutputReqQueue.write(header);
			mCacheLineCursor = 0;

			mCacheFSMState = GP_CACHE_STATE_READ_DATA;
		}

		break;
	}

	case GP_CACHE_STATE_SEND_DATA: {
    // We only start a memory operation if we know there is buffer space to
    // complete it.
    assert(!mOutputReqQueue.full());

	  MemoryRequest payload(MemoryRequest::PAYLOAD_ONLY, mCacheLineBuffer[mCacheLineCursor++]);
	  mOutputReqQueue.write(payload);

	  if (mCacheLineCursor == mWriteBackCount)
	    mCacheFSMState = GP_CACHE_STATE_SEND_READ_COMMAND;

		break;
	}

	case GP_CACHE_STATE_SEND_READ_COMMAND: {
	  // We only start a memory operation if we know there is buffer space to
	  // complete it.
	  assert(!mOutputReqQueue.full());

    if (DEBUG)
      cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes from 0x" << std::hex << mFetchAddress << std::dec << endl;

		MemoryRequest request(MemoryRequest::FETCH_LINE, mFetchAddress, mConfig.LineSize/4, id.tile, id.position);
		mOutputReqQueue.write(request);
		mCacheLineCursor = 0;

		mCacheFSMState = GP_CACHE_STATE_READ_DATA;

		break;
	}

	case GP_CACHE_STATE_READ_DATA: {

	  if (!oRequest.valid() && iResponse.valid() && (iResponseTarget.read() == id.position)) {
	    Word response = iResponse.read();

	    if (DEBUG)
	      cout << this->name() << " received response " << response << endl;

	    mCacheLineBuffer[mCacheLineCursor] = response.toUInt();
	    iResponse.ack();
	    mCacheLineCursor++;

	    mCacheFSMState = (mCacheLineCursor == mFetchCount) ? GP_CACHE_STATE_REPLACE : GP_CACHE_STATE_READ_DATA;
	  }

		break;
	}

	case GP_CACHE_STATE_REPLACE: {
		mGeneralPurposeCacheHandler.replaceCacheLine(mFetchAddress, mCacheLineBuffer);

		mCacheResumeRequest = true;
		mFSMState = mFSMCallbackState;
		break;
	}

	default: {
		assert(false);
		break;
	}
	} // end switch
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
  if (DEBUG) cout << this->name() << " received " << iData.read() << endl;
  assert(iData.valid());
  assert(!mInputQueue.full());

  mInputQueue.write(iData.read());
  iData.ack();
}

void MemoryBank::handleDataOutput() {
  // If we have new data to send:
  if (!mOutputWordPending) {
    if (mOutputQueue.empty()) {
      next_trigger(mOutputQueue.writeEvent());
    }
    else {
      OutputWord outWord = mOutputQueue.read();

      // For 0-7, tile=tile, core=channel, channel=part of request
      // For 8-15, return address is held in channel map table
      ChannelID returnAddr;
      if (outWord.TableIndex < 8)
        returnAddr = ChannelID(id.tile.x, id.tile.y, outWord.TableIndex, outWord.ReturnChannel);
      else
        returnAddr = mChannelMapTable[outWord.TableIndex].ReturnChannel;
      NetworkData word(outWord.Data, returnAddr);
      word.setPortClaim(outWord.PortClaim, false);
      word.setEndOfPacket(outWord.LastWord);
      mOutputWordPending = true;

      // Remove the previous request if we are sending to a new destination.
      // Is this a hack, or is this reasonable?
      if (mActiveOutputWord.channelID().component != word.channelID().component)
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
      Instrumentation::Network::traffic(id, mActiveOutputWord.channelID().component);

    oData.write(mActiveOutputWord);

    // Remove the request for network resources.
    if(mActiveOutputWord.endOfPacket())
      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), false);

    mOutputWordPending = false;

    next_trigger(oData.ack_event());
  }
}


void MemoryBank::requestLoop() {
  switch (mRequestState) {
    case REQ_READY:
      if (iRequest.valid() && (iRequest.read().getOperation() != MemoryRequest::PAYLOAD_ONLY))
        handleNewRequest();
      else
        next_trigger(iRequest.default_event());
      break;
    case REQ_WAITING_FOR_BANKS:
      handleRequestWaitForBanks();
      break;
    case REQ_WAITING_FOR_DATA:
      handleRequestWaitForData();
      break;
    case REQ_FETCH_LINE:
      handleRequestFetch();
      break;
    case REQ_STORE_LINE:
      handleRequestStore();
      break;
  }
}

void MemoryBank::handleNewRequest() {
  // If we've received a request, we're in L2 cache mode, so shouldn't have
  // any L1 operations in the queue.
  assert(mInputQueue.empty());

  MemoryRequest request = iRequest.read();
  assert(request.getOperation() != MemoryRequest::PAYLOAD_ONLY);

  MemoryAddr address = request.getPayload();
  mConfig.Mode = MODE_GP_CACHE;
  mConfig.LineSize = request.getLineSize() * 4;
  mConfig.WayCount = 1;
  mConfig.GroupBaseBank = 0;
  mConfig.GroupIndex = 0;
  mConfig.GroupSize = 1;
  bool targetBank = iRequestTarget.read() == (id.position - CORES_PER_TILE);

  // Ensure this memory bank is configured as an L2 cache bank.
  mGeneralPurposeCacheHandler.activateL2(mConfig);

  // Perform a read to check if the data is currently cached.
  // FIXME: the read is repeated as soon as we enter handleRequestFetch().
  uint32_t data;
  bool cached = mGeneralPurposeCacheHandler.readWord(address, data, false, false, false);

  // We don't hold the data and aren't responsible for fetching it - wait for
  // the next request.
  if (!cached && !targetBank) {
    mRequestState = REQ_READY;
    next_trigger(iRequest.default_event());
  }
  // We don't hold the data but are responsible for fetching it if no other bank
  // has it - wait a cycle to see if any other bank responds.
  else if (!cached && targetBank) {
    mRequestState = REQ_WAITING_FOR_BANKS;
    next_trigger(iClock.negedge_event());
  }
  // We have the cache line and can start the operation.
  else {
    assert(cached);

    // Claim responsibility for handling the request by acknowledging.
    iRequest.ack();
    beginServingRequest(request);
  }
}

void MemoryBank::handleRequestWaitForBanks() {
  assert(mFSMState == STATE_IDLE);

  // If the request is still valid, no other bank has claimed it, so this
  // bank should carry it out. Set up the other state machines to fetch
  // the data and wait until it finishes arriving.
  if (iRequest.valid()) {
    mFSMState = STATE_GP_CACHE_MISS;
    mFSMCallbackState = STATE_IDLE;
    mCacheFSMState = GP_CACHE_STATE_PREPARE;
    mActiveData.Address = iRequest.read().getPayload();

    mRequestState = REQ_WAITING_FOR_DATA;
    next_trigger(iClock.negedge_event());
  }
  // If the request is no longer valid, another bank is servicing it, and we
  // need to do nothing.
  else {
    mRequestState = REQ_READY;
    next_trigger(iRequest.default_event());
  }
}

void MemoryBank::handleRequestWaitForData() {
  // Other state machines are still active and collecting data - wait until
  // they have finished.
  if (mFSMState != STATE_IDLE) {
    next_trigger(iClock.negedge_event());
  }
  else {
    // Claim responsibility for handling the request by acknowledging.
    iRequest.ack();
    beginServingRequest(iRequest.read());
  }
}

void MemoryBank::beginServingRequest(MemoryRequest request) {
  switch (request.getOperation()) {

    case MemoryRequest::FETCH_LINE: {
      mRequestState = REQ_FETCH_LINE;
      next_trigger(sc_core::SC_ZERO_TIME);

      mFetchAddress = request.getPayload();
      mFetchCount = request.getLineSize();

      if (DEBUG)
        cout << this->name() << " reading " << mFetchCount << " words from 0x" << std::hex << mFetchAddress << std::dec << endl;
      break;
    }

    case MemoryRequest::STORE_LINE: {
      mRequestState = REQ_STORE_LINE;
      next_trigger(iRequest.default_event());

      mWriteBackAddress = request.getPayload();
      mWriteBackCount = request.getLineSize();

      if (DEBUG)
        cout << this->name() << " flushing " << mWriteBackCount << " words to 0x" << std::hex << mWriteBackAddress << std::dec << endl;
      break;
    }

    default:
      throw InvalidOptionException("memory bank remote request operation", request.getOperation());
      break;

  } // end switch
}

void MemoryBank::handleRequestFetch() {
  // No buffering - need to wait until the output port is available.
  if (oResponse.valid())
    next_trigger(oResponse.ack_event());
  // Output port is available - send flit.
  else {
    uint32_t data;

    if (DEBUG)
      cout << this->name() << " reading from address 0x" << std::hex << mFetchAddress << std::dec << endl;

    bool hit = mGeneralPurposeCacheHandler.readWord(mFetchAddress, data, false, false, false);
    assert(hit);
    Word response(data);

    mFetchCount--;
    mFetchAddress += 4;
    if (mFetchCount == 0)
      endRequest();
    else
      next_trigger(iClock.negedge_event());

    oResponse.write(response);
  }
}

void MemoryBank::handleRequestStore() {
  assert(iRequest.read().getOperation() == MemoryRequest::PAYLOAD_ONLY);

  if (DEBUG)
    cout << this->name() << " writing " << iRequest.read().getPayload() << " to address 0x" << std::hex << mWriteBackAddress << std::dec << endl;

  bool hit = mGeneralPurposeCacheHandler.writeWord(mWriteBackAddress, iRequest.read().getPayload(), false, false);
  assert(hit);
  iRequest.ack();

  mWriteBackCount--;
  mWriteBackAddress += 4;

  if (mWriteBackCount == 0)
    endRequest();
  else
    next_trigger(iRequest.default_event());
}

void MemoryBank::endRequest() {
  mRequestState = REQ_READY;
  next_trigger(iClock.negedge_event());
}

// Method which sends data from the mOutputReqQueue whenever possible.
void MemoryBank::handleRequestOutput() {
  if (!iClock.posedge())
    next_trigger(iClock.posedge_event());
  else if (mOutputReqQueue.empty())
    next_trigger(mOutputReqQueue.writeEvent());
  else {
    MemoryRequest request = mOutputReqQueue.read();
    if (DEBUG)
      cout << this->name() << " sending request " << request << endl;

    assert(!oRequest.valid());
    oRequest.write(request);
    next_trigger(oRequest.ack_event());
  }
}

void MemoryBank::handleNetworkInterfacesPre() {

  // Set output port defaults - only final write will be effective
  if(oRingStrobe.read())   oRingStrobe.write(false);

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

  if (iClock.posedge()) {

    handleNetworkInterfacesPre();

  }
  else if (iClock.negedge()) {

    // Check ring input ports and update request registers

    if (iRingStrobe.read() && !mRingRequestInputPending) {
      // Acknowledgement handled by separate methods

      mRingRequestInputPending = true;
      mActiveRingRequestInput = iRingRequest.read();
    }

    // Proceed according to FSM state

    // Only allow an operation to begin if we are sure that there is space in
    // the output request queue to hold any potential messages.

    // If we have a "fast" memory, decode the request and get into the correct
    // state before performing the operation.
    // Also, don't allow a new operation to start until any ring requests have
    // been passed on - ensures that data is sent in correct order.
    if (FAST_MEMORY && mFSMState == STATE_IDLE && !mRingRequestOutputPending && mOutputReqQueue.empty()) {
      if (!processRingEvent())
        processMessageHeader();
    }

    switch (mFSMState) {
    case STATE_IDLE:
      // If the memory is not "fast", the operation has to be decoded in one
      // cycle, and performed in the next.
      // Also, don't allow a new operation to start until any ring requests have
      // been passed on - ensures that data is sent in correct order.
      if (!FAST_MEMORY && !mRingRequestOutputPending && mOutputReqQueue.empty()) {
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
      assert(false);
      break;
    }
  }

  // Update status signals

  bool wasIdle = currentlyIdle;
  currentlyIdle = mFSMState == STATE_IDLE &&
                  mInputQueue.empty() && mOutputQueue.empty() &&
                  mOutputReqQueue.empty() &&
                  !mOutputWordPending &&
                  !mRingRequestInputPending && !mRingRequestOutputPending;

  if (wasIdle != currentlyIdle) {
    Instrumentation::idle(id, currentlyIdle);
  }

  bool ready = !mInputQueue.full();
  if (ready != oReadyForData.read())
    oReadyForData.write(ready);
}

//-------------------------------------------------------------------------------------------------
// Constructors and destructors
//-------------------------------------------------------------------------------------------------

MemoryBank::MemoryBank(sc_module_name name, const ComponentID& ID, uint bankNumber) :
	Component(name, ID),
	cChannelMapTableEntries(MEMORY_CHANNEL_MAP_TABLE_ENTRIES),
	cMemoryBanks(MEMS_PER_TILE),
	cBankNumber(bankNumber),
	cRandomReplacement(MEMORY_CACHE_RANDOM_REPLACEMENT != 0),
  mInputQueue(IN_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mInputQueue")),
  mOutputQueue(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mOutputQueue")),
  mOutputReqQueue(10 /*read addr + write addr + cache line*/, string(this->name())+string(".mOutputReqQueue")),
	mScratchpadModeHandler(bankNumber),
	mGeneralPurposeCacheHandler(bankNumber)
{
	//-- Configuration parameters -----------------------------------------------------------------

	assert(cChannelMapTableEntries > 0);
	assert(cMemoryBanks > 0);
	assert(cBankNumber < cMemoryBanks);

	//-- Data queue state -------------------------------------------------------------------------

	mOutputWordPending = false;
	mActiveOutputWord = NetworkData();

	//-- Mode independent state -------------------------------------------------------------------

	mConfig.Mode = MODE_INACTIVE;
	mConfig.WayCount = 0;
	mConfig.LineSize = 0;
	mConfig.GroupBaseBank = 0;
	mConfig.GroupIndex = 0;
	mConfig.GroupSize = 0;

	mChannelMapTable = new ChannelMapTableEntry[cChannelMapTableEntries];
	memset(mChannelMapTable, 0x00, sizeof(ChannelMapTableEntry) * cChannelMapTableEntries);

	mFSMState = STATE_IDLE;
	mFSMCallbackState = STATE_IDLE;

	mActiveRequest = MemoryRequest();
	mActiveData.TableIndex = -1;
	mActiveData.ReturnChannel = -1;
	mActiveData.Address = -1;
	mActiveData.Count = 0;
	mActiveData.PartialInstructionPending = false;
	mActiveData.PartialInstructionData = 0;

	//-- Cache mode state -------------------------------------------------------------------------

	mCacheResumeRequest = false;
	mCacheFSMState = GP_CACHE_STATE_PREPARE;
	mCacheLineCursor = 0;
	mWriteBackAddress = 0;
	mWriteBackCount = 0;
	mFetchAddress = 0;
	mFetchCount = 0;

	//-- L2 cache mode state ----------------------------------------------------------------------

	mRequestState = REQ_READY;

	//-- Ring network state -----------------------------------------------------------------------

	mRingRequestInputPending = false;
	mRingRequestOutputPending = false;

	//-- Debug utilities --------------------------------------------------------------------------

	mPrevMemoryBank = 0;
	mNextMemoryBank = 0;
	mBackgroundMemory = 0;
	localNetwork = 0;

	//-- Port initialization ----------------------------------------------------------------------

	oReadyForData.initialize(false);
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

	SC_METHOD(requestLoop);

  SC_METHOD(handleRequestOutput);

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

// Find the memory bank responsible for storing a given address.
MemoryBank& MemoryBank::bankContainingAddress(MemoryAddr addr) {
  MemoryBank *firstBank = this;
  while (firstBank->mConfig.GroupIndex != 0)
    firstBank = firstBank->mPrevMemoryBank;

  MemoryBank *bank = firstBank;

  for (uint groupIndex = 0; groupIndex < mConfig.GroupSize; groupIndex++) {
    if (bank->currentMemoryHandler().containsAddress(addr))
      return *bank;

    bank = bank->mNextMemoryBank;
  }

  assert(false);
  return *this;
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	size_t count = data.size();
	uint32_t address = location;

	assert(address % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

  for (size_t i = 0; i < count; i++) {
    MemoryBank& bank = bankContainingAddress(address + i*4);
    if (!bank.currentMemoryHandler().writeWord(address + i * 4, data[i].toUInt(), false, true))
      mBackgroundMemory->writeWord(address + i * 4, data[i]);
  }
}

void MemoryBank::synchronizeData() {
	assert(mBackgroundMemory != 0);
	if (mConfig.Mode == MODE_GP_CACHE)
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

	assert(mConfig.Mode != MODE_INACTIVE);

	MemoryBank *firstBank = this;
	while (firstBank->mConfig.GroupIndex != 0)
		firstBank = firstBank->mPrevMemoryBank;

	if (mConfig.Mode == MODE_SCRATCHPAD)
		cout << "Virtual memory group scratchpad data (banks " << (cBankNumber - mConfig.GroupIndex) << " to " << (cBankNumber - mConfig.GroupIndex + mConfig.GroupSize - 1) << "):\n" << endl;
	else
		cout << "Virtual memory group general purpose cache data (banks " << (cBankNumber - mConfig.GroupIndex) << " to " << (cBankNumber - mConfig.GroupIndex + mConfig.GroupSize - 1) << "):\n" << endl;

  while (address < limit) {
    MemoryBank& bank = bankContainingAddress(address);

    uint32_t data;
    bool cached = bank.currentMemoryHandler().readWord(address, data, false, false, true);

    if (!cached)
      data = mBackgroundMemory->readWord(address).toUInt();

    cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << dec;
    if (cached)
      cout << " C";
    cout << endl;

    address += 4;
  }
}

Word MemoryBank::readWord(MemoryAddr addr) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(addr % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

	MemoryBank& bank = bankContainingAddress(addr);
  uint32_t data;
  bool cached = bank.currentMemoryHandler().readWord(addr, data, false, false, true);

  if (!cached)
    data = mBackgroundMemory->readWord(addr).toUInt();

  return Word(data);
}

Word MemoryBank::readByte(MemoryAddr addr) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(mConfig.Mode != MODE_INACTIVE);

  MemoryBank& bank = bankContainingAddress(addr);
  uint32_t data;
  bool cached = bank.currentMemoryHandler().readByte(addr, data, false, true);

  if (!cached)
    data = mBackgroundMemory->readByte(addr).toUInt();

  return Word(data);
}

void MemoryBank::writeWord(MemoryAddr addr, Word data) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(addr % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

  MemoryBank& bank = bankContainingAddress(addr);
  bool cached = bank.currentMemoryHandler().writeWord(addr, data.toUInt(), false, true);

  if (!cached)
    mBackgroundMemory->writeWord(addr, data.toUInt());
}

void MemoryBank::writeByte(MemoryAddr addr, Word data) {
	assert(mPrevMemoryBank != 0 && mNextMemoryBank != 0 && mBackgroundMemory != 0);

	assert(mConfig.Mode != MODE_INACTIVE);

  MemoryBank& bank = bankContainingAddress(addr);
  bool cached = bank.currentMemoryHandler().writeByte(addr, data.toUInt(), false, true);

  if (!cached)
    mBackgroundMemory->writeByte(addr, data.toUInt());
}

void MemoryBank::reportStalls(ostream& os) {
  if (mInputQueue.full()) {
    os << mInputQueue.name() << " is full." << endl;
  }
  if (!mOutputQueue.empty()) {
    const OutputWord& outWord = mOutputQueue.peek();
    ChannelID addr;
    if (outWord.TableIndex < 8)
      addr = ChannelID(id.tile.x, id.tile.y, outWord.TableIndex, outWord.ReturnChannel);
    else
      addr = mChannelMapTable[outWord.TableIndex].ReturnChannel;

    os << this->name() << " waiting to send to " << addr << endl;
  }
}

void MemoryBank::printOperation(MemoryRequest::MemoryOperation operation,
                                MemoryAddr                     address,
                                uint32_t                       data) const {

  bool isLoad;
  string datatype;

  switch (operation) {
  case MemoryRequest::LOAD_W:
    isLoad = true;  datatype = "word";      break;
  case MemoryRequest::LOAD_HW:
    isLoad = true;  datatype = "half-word"; break;
  case MemoryRequest::LOAD_B:
    isLoad = true;  datatype = "byte";      break;
  case MemoryRequest::STORE_W:
  case MemoryRequest::STORE_THROUGH_W:
    isLoad = false; datatype = "word";      break;
  case MemoryRequest::STORE_HW:
  case MemoryRequest::STORE_THROUGH_HW:
    isLoad = false; datatype = "half-word"; break;
  case MemoryRequest::STORE_B:
  case MemoryRequest::STORE_THROUGH_B:
    isLoad = false; datatype = "byte";      break;
  default:
    assert(false);
    break;
  }

  cout << this->name() << (isLoad ? " read " : " wrote ") << datatype << " "
       << data << (isLoad ? " from" : " to") << " 0x" << std::hex << address << std::dec << endl;

}

void MemoryBank::printConfiguration() const {
  uint startBank = mConfig.GroupBaseBank;
  uint endBank = startBank + mConfig.GroupSize - 1;
  stringstream mode;
  if (mConfig.Mode == MODE_SCRATCHPAD)
    mode << "scratchpad";
  else
    mode << "cache (associativity " << mConfig.WayCount << ")";
  uint lineSize = mConfig.LineSize;

  cout << "MEMORY CONFIG: tile " << id.tile << ", banks " << startBank << "-"
      << endBank << ", line size " << lineSize << " bytes, " << mode.str() << endl;
}
