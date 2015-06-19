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
#include "../../Utility/Instrumentation/MemoryBank.h"
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


AbstractMemoryHandler& MemoryBank::currentMemoryHandler() {
  bool checkTags = isL1Request(mActiveRequest)
                 ? mActiveRequest.getMemoryMetadata().checkL1Tags
                 : mActiveRequest.getMemoryMetadata().checkL2Tags;

  if (checkTags)
    return mGeneralPurposeCacheHandler;
  else
    return mScratchpadModeHandler;
}

ComponentID MemoryBank::requestingComponent(const NetworkRequest& request) const {
  if (isL1Request(request)) {
    // In L1 mode, the core is on the same tile and can be determined by the
    // input channel accessed.
    return ComponentID(id.tile, mActiveData.TableIndex);
  }
  else {
    // In L2 mode, the requesting bank is supplied as part of the request.
    return ComponentID(request.getMemoryMetadata().returnTileX,
                       request.getMemoryMetadata().returnTileY,
                       request.getMemoryMetadata().returnChannel);
  }
}

ChannelID MemoryBank::returnChannel(const NetworkRequest& request) const {
  if (isL1Request(request)) {
    // In L1 mode, the core is on the same tile and can be determined by the
    // input channel accessed.
    return ChannelID(id.tile.x,
                     id.tile.y,
                     mActiveData.TableIndex,
                     request.getMemoryMetadata().returnChannel);
  }
  else {
    // In L2 mode, the requesting bank is supplied as part of the request.
    return ChannelID(request.getMemoryMetadata().returnTileX,
                     request.getMemoryMetadata().returnTileY,
                     request.getMemoryMetadata().returnChannel,
                     0);
  }
}

bool MemoryBank::isL1Request(const NetworkRequest& request) const {
  // Perform a naive test on the active request. The returnTile fields are
  // not set by requests from cores.
  // Alternatively, may prefer to choose based on which queue the request was
  // taken from.
  return (request.getMemoryMetadata().returnTileX == 0);
}

bool MemoryBank::endOfCacheLine(MemoryAddr address) const {
  // Cache line = 32 bytes = 2^5 bytes.
  uint mask = 0xFFFFFFE0;

  return (address & mask) != ((address+4) & mask);
}

bool MemoryBank::processMessageHeader() {
  mFSMState = STATE_IDLE;

  if (mInputQueue.empty())
		return false;

  NetworkRequest request = mInputQueue.peek();

  if (DEBUG)
    cout << this->name() << " dequeued " << request << endl;

	mActiveData.TableIndex = request.channelID().channel;
	mActiveData.ReturnChannel = request.getMemoryMetadata().returnChannel;
	mActiveRequest = request;
	bool inputWordProcessed = true;

	switch (mActiveRequest.getMemoryMetadata().opcode) {
	case MemoryRequest::CONTROL: {
		if (DEBUG)
			cout << this->name() << " starting CONTROL request on channel " << mActiveData.TableIndex << endl;

    MemoryRequest configuration = mActiveRequest.payload();

    mConfig.WayCount = 1UL << configuration.getWayBits();
    mConfig.LineSize = CACHE_LINE_WORDS * BYTES_PER_WORD;

    mConfig.GroupSize = 1UL << configuration.getGroupBits();

    // TODO: have something similar to this code before any data access.
    // Remember to check whether we are in L1 or L2 mode.
//			if (mActiveRequest.getMemoryMetadata().checkL1Tags) {
      mConfig.Mode = MODE_GP_CACHE;
      mGeneralPurposeCacheHandler.activate(mConfig);
//			} else {
//				mConfig.Mode = MODE_SCRATCHPAD;
//				mScratchpadModeHandler.activate(mConfig);
//			}

    // Invalidate all atomic transactions.
    mReservations.clearReservationRange(0, 0xFFFFFFFF);

    mFSMState = STATE_IDLE;

		break;
	}

	case MemoryRequest::LOAD_W:
	case MemoryRequest::LOAD_HW:
	case MemoryRequest::LOAD_B:
	case MemoryRequest::STORE_W:
	case MemoryRequest::STORE_HW:
	case MemoryRequest::STORE_B:
	case MemoryRequest::LOAD_LINKED:
	case MemoryRequest::STORE_CONDITIONAL:
	case MemoryRequest::LOAD_AND_ADD:
	case MemoryRequest::LOAD_AND_OR:
	case MemoryRequest::LOAD_AND_AND:
	case MemoryRequest::LOAD_AND_XOR:
	case MemoryRequest::EXCHANGE:
		if (DEBUG)
			cout << this->name() << " starting scalar request on channel " << mActiveData.TableIndex << endl;

		mActiveData.Address = mActiveRequest.payload().toUInt();
		mFSMState = STATE_LOCAL_MEMORY_ACCESS;

		break;

	case MemoryRequest::IPK_READ:
		if (DEBUG)
			cout << this->name() << " starting IPK_READ request on channel " << mActiveData.TableIndex << endl;

		if (ENERGY_TRACE)
		  Instrumentation::MemoryBank::initiateIPKRead(cBankNumber, false);

		mActiveData.Address = mActiveRequest.payload().toUInt();
		mFSMState = STATE_LOCAL_IPK_READ;

		break;

	case MemoryRequest::UPDATE_DIRECTORY_ENTRY:
    if (DEBUG) {
      cout << this->name() << " starting DIRECTORY_UPDATE request on channel " << mActiveData.TableIndex << endl;
      cout << this->name() << " buffering request to update directory: " << mActiveRequest.payload() << endl;
    }

    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(mActiveRequest);
    mFSMState = STATE_IDLE;
    break;

	case MemoryRequest::UPDATE_DIRECTORY_MASK:
    if (DEBUG) {
      cout << this->name() << " starting DIRECTORY_MASK_UPDATE request on channel " << mActiveData.TableIndex << endl;
      cout << this->name() << " buffering request to update directory mask: " << mActiveRequest.payload() << endl;
    }

    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(mActiveRequest);
    mFSMState = STATE_IDLE;
	  break;

	default:
		cout << this->name() << " starting invalid memory request type (" << mActiveRequest.getMemoryMetadata().opcode << ")" << endl;
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

void MemoryBank::processLocalMemoryAccess() {
	assert(mActiveData.TableIndex < 8);

	if (mOutputQueue.full()/* && (mActiveRequest.isSingleLoad())*/) {
		// Delay /*load*/ request until there is room in the output queue available

		if (DEBUG)
			cout << this->name() << " delayed scalar request due to full output queue" << endl;

		return;
	} else {
	  AbstractMemoryHandler& handler = currentMemoryHandler();

		// Loads
		if (MemoryRequest::isSingleLoad(mActiveRequest.getMemoryMetadata().opcode)) {
			bool cacheHit;
			uint32_t data;

			switch (mActiveRequest.getMemoryMetadata().opcode) {
			case MemoryRequest::LOAD_W:		cacheHit = handler.readWord(mActiveData.Address, data, false, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);	break;
			case MemoryRequest::LOAD_HW:	cacheHit = handler.readHalfWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);		break;
			case MemoryRequest::LOAD_B:		cacheHit = handler.readByte(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);			break;
			case MemoryRequest::LOAD_LINKED:
			  cacheHit = handler.readWord(mActiveData.Address, data, false, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);
			  if (cacheHit)
			    mReservations.makeReservation(requestingComponent(mActiveRequest), mActiveData.Address);
			  break;
			default:						assert(false);																								break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			if (cacheHit) {
				if (DEBUG)
				  printOperation(mActiveRequest.getMemoryMetadata().opcode,
				                 mActiveData.Address, data);

				// Enqueue output request
			  NetworkResponse output = assembleResponse(data, true);
				mOutputQueue.write(output);

				// Chain next request
				processMessageHeader();
			} else {
				mFSMCallbackState = STATE_LOCAL_MEMORY_ACCESS;
				mFSMState = STATE_GP_CACHE_MISS;
				mCacheFSMState = GP_CACHE_STATE_PREPARE;
			}

		// Stores
		} else if (MemoryRequest::isSingleStore(mActiveRequest.getMemoryMetadata().opcode)) {

			if (mInputQueue.empty())
				return;

			bool cacheHit;
			NetworkRequest flit = mInputQueue.peek();

			if (flit.getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY) {
				cout << "!!!! " << mActiveRequest.getMemoryMetadata().opcode << " | " << mActiveData.Address << " | " << flit.getMemoryMetadata().opcode << " | " << flit.payload() << endl;
			}

			// The received word should be the continuation of the previous operation.
			assert(flit.getMemoryMetadata().opcode == MemoryRequest::PAYLOAD_ONLY);

			switch (mActiveRequest.getMemoryMetadata().opcode) {
			case MemoryRequest::STORE_W:	cacheHit = handler.writeWord(mActiveData.Address, flit.payload().toUInt(), mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);		break;
			case MemoryRequest::STORE_HW:	cacheHit = handler.writeHalfWord(mActiveData.Address, flit.payload().toUInt(), mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);	break;
			case MemoryRequest::STORE_B:	cacheHit = handler.writeByte(mActiveData.Address, flit.payload().toUInt(), mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);		break;
			case MemoryRequest::STORE_CONDITIONAL: {
			  bool reservationValid = mReservations.checkReservation(requestingComponent(mActiveRequest), mActiveData.Address);
			  if (reservationValid)
			    cacheHit = handler.writeWord(mActiveData.Address, flit.payload().toUInt(), mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);
			  else
			    cacheHit = true;  // The operation has finished; continue with the next one.

			  // Return whether the store was successful.
			  if (cacheHit) {
			    NetworkResponse output = assembleResponse(reservationValid, true);
			    mOutputQueue.write(output);
			  }

			  break;
			}
			default:						assert(false);																											break;
			}

			assert(!(mCacheResumeRequest && !cacheHit));
			mCacheResumeRequest = false;

			if (cacheHit) {
				// Remove the request from the queue after completing it.
				mInputQueue.read();

				// Invalidate any atomic transactions relying on the overwritten data.
				mReservations.clearReservation(mActiveData.Address);

				if (DEBUG) {
				  cout << this->name() << " dequeued " << flit << endl;
				  printOperation(mActiveRequest.getMemoryMetadata().opcode, mActiveData.Address, flit.payload().toUInt());
				}

				// Chain next request
				processMessageHeader();
			} else {
				mFSMCallbackState = STATE_LOCAL_MEMORY_ACCESS;
				mFSMState = STATE_GP_CACHE_MISS;
				mCacheFSMState = GP_CACHE_STATE_PREPARE;
			}

		// Read-modify-writes
		} else if (MemoryRequest::isReadModifyWrite(mActiveRequest.getMemoryMetadata().opcode)) {

		  // First half: read data.
		  if (!mRMWDataValid) {
        bool cacheHit;
        uint32_t data;
        switch (mActiveRequest.getMemoryMetadata().opcode) {
          case MemoryRequest::LOAD_AND_ADD:
          case MemoryRequest::LOAD_AND_OR:
          case MemoryRequest::LOAD_AND_AND:
          case MemoryRequest::LOAD_AND_XOR:
          case MemoryRequest::EXCHANGE:
            cacheHit = handler.readWord(mActiveData.Address, data, false, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);
            break;
          default:
            assert(false); break;
        }

        assert(!(mCacheResumeRequest && !cacheHit));
        mCacheResumeRequest = false;

        if (cacheHit) {
          mRMWData = data;
          mRMWDataValid = true;

          if (DEBUG)
            printOperation(MemoryRequest::LOAD_W, mActiveData.Address, data);

          // Enqueue output request
          NetworkResponse output = assembleResponse(data, true);
          mOutputQueue.write(output);
        } else {
          mFSMCallbackState = STATE_LOCAL_MEMORY_ACCESS;
          mFSMState = STATE_GP_CACHE_MISS;
          mCacheFSMState = GP_CACHE_STATE_PREPARE;
        }
		  }

      // Second half: modify and write data.
		  else {
        if (mInputQueue.empty())
          return;

        NetworkRequest flit = mInputQueue.read();

        if (flit.getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY) {
          cout << "!!!! " << mActiveRequest.getMemoryMetadata().opcode << " | " << mActiveData.Address << " | " << flit.getMemoryMetadata().opcode << " | " << flit.payload() << endl;
        }

        // The received word should be the continuation of the previous operation.
        assert(flit.getMemoryMetadata().opcode == MemoryRequest::PAYLOAD_ONLY);

        uint32_t data = flit.payload().toUInt();
        switch (mActiveRequest.getMemoryMetadata().opcode) {
          case MemoryRequest::LOAD_AND_ADD:
            data += mRMWData; break;
          case MemoryRequest::LOAD_AND_OR:
            data |= mRMWData; break;
          case MemoryRequest::LOAD_AND_AND:
            data &= mRMWData; break;
          case MemoryRequest::LOAD_AND_XOR:
            data ^= mRMWData; break;
          case MemoryRequest::EXCHANGE:
            break;
          default:
            assert(false); break;
        }

        bool cacheHit = handler.writeWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);

        // There cannot have been any operations between the load from this
        // address and this store.
        assert(cacheHit);
        mCacheResumeRequest = false;
        mRMWDataValid = false;

        // Invalidate any atomic transactions relying on the overwritten data.
        mReservations.clearReservation(mActiveData.Address);

        if (DEBUG) {
          cout << this->name() << " dequeued " << flit << endl;
          printOperation(MemoryRequest::STORE_W, mActiveData.Address, flit.payload().toUInt());
        }

        // Chain next request
        processMessageHeader();
		  }
		}
		else {
			assert(false);
		}
	}
}

void MemoryBank::processLocalIPKRead() {
	assert(mActiveData.TableIndex < 8);
	assert(mActiveRequest.getMemoryMetadata().opcode == MemoryRequest::IPK_READ);

	if (mOutputQueue.full()) {
		// Delay load request until there is room in the output queue available
		return;
	} else {
		if (DEBUG)
			cout << this->name() << " reading instruction at address 0x" << std::hex << mActiveData.Address << std::dec << " from local cache" << endl;

		AbstractMemoryHandler& handler = currentMemoryHandler();

		uint32_t data;
		bool cacheHit = handler.readWord(mActiveData.Address, data, true, mCacheResumeRequest, false, mActiveData.TableIndex, mActiveData.ReturnChannel);

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

  // Enqueue output response
  NetworkResponse output = assembleResponse(data, endOfPacket || endOfCacheLine(mActiveData.Address));
  mOutputQueue.write(output);

  // Handle IPK streaming

  if (endOfPacket || endOfCacheLine(mActiveData.Address)) {
    if (endOfPacket && Arguments::memoryTrace())
      MemoryTrace::stopIPK(cBankNumber, mActiveData.Address);

    if (endOfCacheLine(mActiveData.Address) && Arguments::memoryTrace())
      MemoryTrace::splitLineIPK(cBankNumber, mActiveData.Address);

    // Chain next request
    processMessageHeader();
  } else {
    mActiveData.Address += 4;
    mFSMState = STATE_LOCAL_IPK_READ;
  }
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

			if (DEBUG)
			  cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes to 0x" << std::hex << mFetchAddress << std::dec << endl;

			NetworkRequest header(mWriteBackAddress, id, MemoryRequest::STORE_LINE, false);
			mOutputReqQueue.write(header);

			mCacheFSMState = GP_CACHE_STATE_SEND_DATA;
		} else {
			assert(mFetchCount == mConfig.LineSize / 4);

			if (DEBUG)
			  cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes from 0x" << std::hex << mFetchAddress << std::dec << endl;

			NetworkRequest header(mFetchAddress, id, MemoryRequest::FETCH_LINE, true);
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

    uint32_t data = mCacheLineBuffer[mCacheLineCursor++];
    bool endOfPacket = (mCacheLineCursor == mWriteBackCount);

	  if (DEBUG)
	    cout << this->name() << " buffering request data " << data << endl;

	  NetworkRequest payload(data, id, MemoryRequest::PAYLOAD_ONLY, endOfPacket);
	  mOutputReqQueue.write(payload);

	  if (endOfPacket)
	    mCacheFSMState = GP_CACHE_STATE_SEND_READ_COMMAND;

		break;
	}

	case GP_CACHE_STATE_SEND_READ_COMMAND: {
	  // We only start a memory operation if we know there is buffer space to
	  // complete it.
	  assert(!mOutputReqQueue.full());

    if (DEBUG)
      cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes from 0x" << std::hex << mFetchAddress << std::dec << endl;

    NetworkRequest request(mFetchAddress, id, MemoryRequest::FETCH_LINE, true);
		mOutputReqQueue.write(request);
		mCacheLineCursor = 0;

		mCacheFSMState = GP_CACHE_STATE_READ_DATA;

		break;
	}

	case GP_CACHE_STATE_READ_DATA: {

	  if (!oRequest.valid() && iResponse.valid() && (iResponseTarget.read() == id.position-CORES_PER_TILE)) {
	    Word response = iResponse.read().payload();

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

    // Invalidate any atomic transactions relying on the overwritten data.
    mReservations.clearReservationRange(mFetchAddress, mFetchAddress+mConfig.LineSize);

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

void MemoryBank::processValidInput() {
  if (DEBUG) cout << this->name() << " received " << iData.read() << endl;
  assert(iData.valid());
  assert(!mInputQueue.full());

  mInputQueue.write(iData.read());
  iData.ack();
}

NetworkResponse MemoryBank::assembleResponse(uint32_t data, bool endOfPacket) {
  return NetworkResponse(data, returnChannel(mActiveRequest), endOfPacket);
}

void MemoryBank::handleDataOutput() {
  // If we have new data to send:
  if (!mOutputWordPending) {
    if (mOutputQueue.empty()) {
      next_trigger(mOutputQueue.writeEvent());
    }
    else {
      NetworkResponse flit = mOutputQueue.read();
      mOutputWordPending = true;

      // Remove the previous request if we are sending to a new destination.
      // Is this a hack, or is this reasonable?
      if (mActiveOutputWord.channelID().component != flit.channelID().component)
        localNetwork->makeRequest(id, mActiveOutputWord.channelID(), false);

      mActiveOutputWord = flit;

      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), true);
      next_trigger(iClock.negedge_event());
    }
  }
  // Check to see whether we are allowed to send data yet.
  else if (!localNetwork->requestGranted(id, mActiveOutputWord.channelID())) {
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
    if (mActiveOutputWord.getMetadata().endOfPacket)
      localNetwork->makeRequest(id, mActiveOutputWord.channelID(), false);

    mOutputWordPending = false;

    next_trigger(oData.ack_event());
  }
}


// TODO: merge these operations in with the ones received from cores.
void MemoryBank::requestLoop() {
  switch (mRequestState) {
    case REQ_READY:
      if (iRequest.valid() && (iRequest.read().getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY))
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

  NetworkRequest request = iRequest.read();
  assert(request.getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY);

  MemoryAddr address = request.payload().toUInt();
  mConfig.Mode = MODE_GP_CACHE;
  mConfig.LineSize = CACHE_LINE_WORDS * 4;
  mConfig.WayCount = 1;
  mConfig.GroupSize = 1;
  bool targetBank = iRequestTarget.read() == (id.position - CORES_PER_TILE);

  // Ensure this memory bank is configured as an L2 cache bank.
  mGeneralPurposeCacheHandler.activateL2(mConfig);

  // Perform a read to check if the data is currently cached.
  // FIXME: the read is repeated as soon as we enter handleRequestFetch().
  uint32_t data;
  bool cached = mGeneralPurposeCacheHandler.readWord(address, data, false, false, false, 0, 0);

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
    mActiveData.Address = iRequest.read().payload().toUInt();

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

void MemoryBank::beginServingRequest(NetworkRequest request) {
  mActiveRequest = request;

  switch (request.getMemoryMetadata().opcode) {

    case MemoryRequest::FETCH_LINE: {
      mRequestState = REQ_FETCH_LINE;
      next_trigger(sc_core::SC_ZERO_TIME);

      mFetchAddress = request.payload().toUInt();
      mFetchCount = CACHE_LINE_WORDS;

      if (DEBUG)
        cout << this->name() << " reading " << mFetchCount << " words from 0x" << std::hex << mFetchAddress << std::dec << endl;
      break;
    }

    case MemoryRequest::STORE_LINE: {
      mRequestState = REQ_STORE_LINE;
      next_trigger(iRequest.default_event());

      mWriteBackAddress = request.payload().toUInt();
      mWriteBackCount = CACHE_LINE_WORDS;

      if (DEBUG)
        cout << this->name() << " flushing " << mWriteBackCount << " words to 0x" << std::hex << mWriteBackAddress << std::dec << endl;
      break;
    }

    default:
      throw InvalidOptionException("memory bank remote request operation", request.getMemoryMetadata().opcode);
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

    bool hit = mGeneralPurposeCacheHandler.readWord(mFetchAddress, data, false, false, false, 0, 0);
    assert(hit);

    mFetchCount--;
    mFetchAddress += 4;
    bool endOfPacket = (mFetchCount == 0);

    NetworkResponse response = assembleResponse(data, endOfPacket);

    if (endOfPacket)
      endRequest();
    else
      next_trigger(iClock.negedge_event());

    oResponse.write(response);
  }
}

void MemoryBank::handleRequestStore() {
  NetworkRequest request = iRequest.read();
  assert(request.getMemoryMetadata().opcode == MemoryRequest::PAYLOAD_ONLY);

  if (DEBUG)
    cout << this->name() << " writing " << request.payload() << " to address 0x" << std::hex << mWriteBackAddress << std::dec << endl;

  bool hit = mGeneralPurposeCacheHandler.writeWord(mWriteBackAddress, request.payload().toUInt(), false, false, 0, 0);
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
    NetworkRequest request = mOutputReqQueue.read();
    if (DEBUG)
      cout << this->name() << " sending request " << request.payload() << endl;

    assert(!oRequest.valid());
    oRequest.write(request);
    next_trigger(oRequest.ack_event());
  }
}

void MemoryBank::mainLoop() {

  if (iClock.negedge()) {

    // Proceed according to FSM state

    // Only allow an operation to begin if we are sure that there is space in
    // the output request queue to hold any potential messages.

    // If we have a "fast" memory, decode the request and get into the correct
    // state before performing the operation.
    // Also, don't allow a new operation to start until any ring requests have
    // been passed on - ensures that data is sent in correct order.
    if (FAST_MEMORY && mFSMState == STATE_IDLE && mOutputReqQueue.empty()) {
      processMessageHeader();
    }

    switch (mFSMState) {
    case STATE_IDLE:
      // If the memory is not "fast", the operation has to be decoded in one
      // cycle, and performed in the next.
      // Also, don't allow a new operation to start until any ring requests have
      // been passed on - ensures that data is sent in correct order.
      if (!FAST_MEMORY  && mOutputReqQueue.empty()) {
        processMessageHeader();
      }

      break;

    case STATE_LOCAL_MEMORY_ACCESS:
      processLocalMemoryAccess();
      break;

    case STATE_LOCAL_IPK_READ:
      processLocalIPKRead();
      break;

    case STATE_GP_CACHE_MISS:
      processGeneralPurposeCacheMiss();
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
                  !mOutputWordPending;

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
	cMemoryBanks(MEMS_PER_TILE),
	cBankNumber(bankNumber),
	cRandomReplacement(MEMORY_CACHE_RANDOM_REPLACEMENT != 0),
  mInputQueue(IN_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mInputQueue")),
  mOutputQueue(OUT_CHANNEL_BUFFER_SIZE, string(this->name())+string(".mOutputQueue")),
  mOutputReqQueue(10 /*read addr + write addr + cache line*/, string(this->name())+string(".mOutputReqQueue")),
  mReservations(1),
	mScratchpadModeHandler(bankNumber),
	mGeneralPurposeCacheHandler(bankNumber)
{
	//-- Configuration parameters -----------------------------------------------------------------

	assert(cMemoryBanks > 0);
	assert(cBankNumber < cMemoryBanks);

	//-- Data queue state -------------------------------------------------------------------------

	mOutputWordPending = false;
	mActiveOutputWord = NetworkData();

	//-- Mode independent state -------------------------------------------------------------------

	mConfig.Mode = MODE_GP_CACHE;
	mConfig.WayCount = 1;
	mConfig.LineSize = CACHE_LINE_WORDS * BYTES_PER_WORD;
	mConfig.GroupSize = MEMS_PER_TILE;
	mGeneralPurposeCacheHandler.activate(mConfig);
	mScratchpadModeHandler.activate(mConfig);

	mFSMState = STATE_IDLE;
	mFSMCallbackState = STATE_IDLE;

	mActiveRequest = NetworkRequest();
	mActiveData.TableIndex = -1;
	mActiveData.ReturnChannel = -1;
	mActiveData.Address = -1;
	mActiveData.Count = 0;

	mRMWData = 0;
	mRMWDataValid = false;

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

	//-- Debug utilities --------------------------------------------------------------------------

	mBackgroundMemory = 0;
	localNetwork = 0;

	//-- Port initialization ----------------------------------------------------------------------

	oReadyForData.initialize(false);

	//-- Register module with SystemC simulation kernel -------------------------------------------

	currentlyIdle = true;
	Instrumentation::idle(id, true);

	SC_METHOD(mainLoop);
	sensitive << iClock;
	dont_initialize();

	SC_METHOD(processValidInput);
	sensitive << iData;
	dont_initialize();

	SC_METHOD(handleDataOutput);

	SC_METHOD(requestLoop);

  SC_METHOD(handleRequestOutput);

	end_module(); // Needed because we're using a different Component constructor
}

MemoryBank::~MemoryBank() {
}

void MemoryBank::setLocalNetwork(local_net_t* network) {
  localNetwork = network;
}

void MemoryBank::setBackgroundMemory(SimplifiedOnChipScratchpad* memory) {
  mBackgroundMemory = memory;
  mGeneralPurposeCacheHandler.setBackgroundMemory(memory);
}

// Find the memory bank responsible for storing a given address.
MemoryBank& MemoryBank::bankContainingAddress(MemoryAddr addr) {
  return *this;
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(mBackgroundMemory != NULL);

	size_t count = data.size();
	uint32_t address = location;

	assert(address % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

  for (size_t i = 0; i < count; i++) {
    MemoryBank& bank = bankContainingAddress(address + i*4);
    if (!bank.currentMemoryHandler().writeWord(address + i * 4, data[i].toUInt(), false, true, 0, 0))
      mBackgroundMemory->writeWord(address + i * 4, data[i]);
  }
}

void MemoryBank::synchronizeData() {
	assert(mBackgroundMemory != NULL);
	if (mConfig.Mode == MODE_GP_CACHE)
		mGeneralPurposeCacheHandler.synchronizeData(mBackgroundMemory);
}

void MemoryBank::print(MemoryAddr start, MemoryAddr end) {
	assert(mBackgroundMemory != NULL);

	if (start > end) {
		MemoryAddr temp = start;
		start = end;
		end = temp;
	}

	size_t address = (start / 4) * 4;
	size_t limit = (end / 4) * 4 + 4;

	assert(mConfig.Mode != MODE_INACTIVE);

  while (address < limit) {
    MemoryBank& bank = bankContainingAddress(address);

    uint32_t data;
    bool cached = bank.currentMemoryHandler().readWord(address, data, false, false, true, 0, 0);

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
	assert(mBackgroundMemory != NULL);

	assert(addr % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

	MemoryBank& bank = bankContainingAddress(addr);
  uint32_t data;
  bool cached = bank.currentMemoryHandler().readWord(addr, data, false, false, true, 0, 0);

  if (!cached)
    data = mBackgroundMemory->readWord(addr).toUInt();

  return Word(data);
}

Word MemoryBank::readByte(MemoryAddr addr) {
	assert(mBackgroundMemory != NULL);

	assert(mConfig.Mode != MODE_INACTIVE);

  MemoryBank& bank = bankContainingAddress(addr);
  uint32_t data;
  bool cached = bank.currentMemoryHandler().readByte(addr, data, false, true, 0, 0);

  if (!cached)
    data = mBackgroundMemory->readByte(addr).toUInt();

  return Word(data);
}

void MemoryBank::writeWord(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);

	assert(addr % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

  MemoryBank& bank = bankContainingAddress(addr);
  bool cached = bank.currentMemoryHandler().writeWord(addr, data.toUInt(), false, true, 0, 0);

  if (!cached)
    mBackgroundMemory->writeWord(addr, data.toUInt());
}

void MemoryBank::writeByte(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);

	assert(mConfig.Mode != MODE_INACTIVE);

  MemoryBank& bank = bankContainingAddress(addr);
  bool cached = bank.currentMemoryHandler().writeByte(addr, data.toUInt(), false, true, 0, 0);

  if (!cached)
    mBackgroundMemory->writeByte(addr, data.toUInt());
}

void MemoryBank::reportStalls(ostream& os) {
  if (mInputQueue.full()) {
    os << mInputQueue.name() << " is full." << endl;
  }
  if (!mOutputQueue.empty()) {
    const NetworkResponse& outWord = mOutputQueue.peek();

    os << this->name() << " waiting to send to " << outWord.channelID() << endl;
  }
}

void MemoryBank::printOperation(MemoryRequest::MemoryOperation operation,
                                MemoryAddr                     address,
                                uint32_t                       data) const {

  bool isLoad;
  string datatype;

  // Note: read-modify-write operations are printed in two phases: as a read,
  // then a write.

  switch (operation) {
  case MemoryRequest::LOAD_W:
  case MemoryRequest::LOAD_LINKED:
    isLoad = true;  datatype = "word";      break;
  case MemoryRequest::LOAD_HW:
    isLoad = true;  datatype = "halfword";  break;
  case MemoryRequest::LOAD_B:
    isLoad = true;  datatype = "byte";      break;
  case MemoryRequest::STORE_W:
  case MemoryRequest::STORE_CONDITIONAL:
    isLoad = false; datatype = "word";      break;
  case MemoryRequest::STORE_HW:
    isLoad = false; datatype = "halfword";  break;
  case MemoryRequest::STORE_B:
    isLoad = false; datatype = "byte";      break;
  default:
    assert(false);
    break;
  }

  cout << this->name() << (isLoad ? " read " : " wrote ") << datatype << " "
       << data << (isLoad ? " from" : " to") << " 0x" << std::hex << address << std::dec << endl;

}
