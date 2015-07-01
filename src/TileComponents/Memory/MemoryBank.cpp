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
#include "../../Exceptions/InvalidOptionException.h"
#include "../../Exceptions/UnsupportedFeatureException.h"

// The latency of accessing a memory bank, aside from the cost of accessing the
// SRAM. This typically includes the network to/from the bank.
#define EXTERNAL_LATENCY 1

// Additional latency to apply to get the total memory latency to match the
// L1_LATENCY parameter. Includes one extra cycle of unavoidable latency within
// the bank.
#define INTERNAL_LATENCY (L1_LATENCY - EXTERNAL_LATENCY - 1)

//-------------------------------------------------------------------------------------------------
// Internal functions
//-------------------------------------------------------------------------------------------------


AbstractMemoryHandler& MemoryBank::currentMemoryHandler() {
  if (checkTags())
    return mGeneralPurposeCacheHandler;
  else
    return mScratchpadModeHandler;
}

bool MemoryBank::checkTags() const {
  if (inL1Mode())
    return !mActiveRequest.getMemoryMetadata().scratchpadL1;
  else
    return !mActiveRequest.getMemoryMetadata().scratchpadL2;
}

ChannelID MemoryBank::returnChannel(const NetworkRequest& request) const {
  switch (mActiveData.Source) {
    // In L1 mode, the core is on the same tile and can be determined by the
    // input channel accessed.
    case REQUEST_CORES:
      return ChannelID(id.tile.x,
                       id.tile.y,
                       request.channelID().channel,
                       request.getMemoryMetadata().returnChannel);
    // In L2 mode, the requesting bank is supplied as part of the request.
    case REQUEST_MEMORIES:
      return ChannelID(request.getMemoryMetadata().returnTileX,
                       request.getMemoryMetadata().returnTileY,
                       request.getMemoryMetadata().returnChannel,
                       0);
    // Refills don't produce results, so we don't need a valid channel.
    default:
      return ChannelID(2,0,0,0);
  }
}

bool MemoryBank::inL1Mode() const {
  return (mActiveData.Source == REQUEST_CORES);
}

bool MemoryBank::onlyAcceptingRefills() const {
  return /*!HIT_UNDER_MISS &&*/ (mFSMCallbackState != STATE_IDLE);
}

MemoryAddr MemoryBank::getTag(MemoryAddr address) const {
  // Cache line = 32 bytes = 2^5 bytes.
  return (address & 0xFFFFFFE0);
}

bool MemoryBank::sameCacheLine(MemoryAddr address1, MemoryAddr address2) const {
  return getTag(address1) == getTag(address2);
}

bool MemoryBank::endOfCacheLine(MemoryAddr address) const {
  return !sameCacheLine(address, address+4);
}

bool MemoryBank::storedLocally(MemoryAddr address) const {
  if (checkTags())
    return mGeneralPurposeCacheHandler.lookupCacheLine(address);
  else
    return true;
}

bool MemoryBank::startNewRequest() {
  assert(mActiveRequestComplete);
  mFSMState = STATE_IDLE;
  mActiveData.Source = REQUEST_NONE;

  // If we were waiting to see if we should serve an L2 request, but the
  // request has now gone, stop waiting.
  if (mWaitingForL2Consensus && !iRequest.valid())
    mWaitingForL2Consensus = false;

  if (!inputAvailable())
		return false;

  // TODO: if allowing hit-under-miss, do a tag look-up to determine if hit or not.

  mActiveRequest = peekInput();

  // In an L2 cache, all banks check their tags. If one of the banks holds
  // the data, it acknowledges the request and takes control. The target bank
  // waits for one cycle to see if any other bank takes control, and if not,
  // takes control itself.
  if (mActiveData.Source == REQUEST_MEMORIES &&
      mActiveRequest.getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY) {
    if (!mWaitingForL2Consensus) {
      bool cacheHit = storedLocally(mActiveRequest.payload().toUInt());

      if (!cacheHit) {
        bool targetBank = iRequestTarget.read() == (id.position - CORES_PER_TILE);
        if (targetBank) {
          mWaitingForL2Consensus = true;
          next_trigger(iClock.negedge_event());
        }
        return false;
      }
    }
  }

  consumeInput();
  return processMessageHeader(mActiveRequest);
}

bool MemoryBank::processMessageHeader(const NetworkRequest& request) {
  MemoryRequest::MemoryOperation opcode = request.getMemoryMetadata().opcode;

  mFSMState = STATE_LOCAL_MEMORY_ACCESS;

  mActiveData.Address = request.payload().toUInt();
	mActiveData.ReturnChannel = returnChannel(request);
	mActiveData.FlitsSent = 0;
	mActiveRequest = request;
	mActiveRequestComplete = false;
	mCacheLineCursor = 0;

  if (DEBUG)
    cout << this->name() << " starting " << MemoryRequest::name(opcode) << " request from component " << mActiveData.ReturnChannel.component << endl;

	switch (opcode) {
	case MemoryRequest::CONTROL: {
		MemoryRequest configuration = mActiveRequest.payload();

    mConfig.WayCount = 1UL << configuration.getWayBits();
    mConfig.LineSize = CACHE_LINE_WORDS * BYTES_PER_WORD;
    mConfig.GroupSize = 1UL << configuration.getGroupBits();

    // TODO: have something similar to this code before any data access.
    // Remember to check whether we are in L1 or L2 mode.
    // Add SRAMAddress to mActiveData and set it above?
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
    mActiveRequestComplete = true;

		break;
	}

	case MemoryRequest::IPK_READ:
		if (ENERGY_TRACE)
		  Instrumentation::MemoryBank::initiateIPKRead(cBankNumber);
		break;

	case MemoryRequest::UPDATE_DIRECTORY_ENTRY:
	case MemoryRequest::UPDATE_DIRECTORY_MASK:
	  // Forward the request on to the tile's directory.
    if (DEBUG)
      cout << this->name() << " buffering directory update request: " << mActiveRequest.payload() << endl;

    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(mActiveRequest);
    mFSMState = STATE_IDLE;
    mActiveRequestComplete = true;
    break;

	case MemoryRequest::FETCH_LINE:
	case MemoryRequest::FLUSH_LINE:
    if (ENERGY_TRACE)
      Instrumentation::MemoryBank::initiateBurstRead(cBankNumber);
    break;

	case MemoryRequest::STORE_LINE:
	case MemoryRequest::MEMSET_LINE:
	  if (ENERGY_TRACE)
	    Instrumentation::MemoryBank::initiateBurstWrite(cBankNumber);
	  mCacheLineBufferTag = getTag(mActiveData.Address);
    break;

	default:
		break;
	}

	return true;
}

void MemoryBank::processLocalMemoryAccess() {
	if (!canSend()) {
		if (DEBUG)
			cout << this->name() << " delayed request due to full output queue" << endl;
		return;
	}

  switch (mActiveRequest.getMemoryMetadata().opcode) {
    case MemoryRequest::LOAD_W:             processLoadWord();          break;
    case MemoryRequest::LOAD_HW:            processLoadHalfWord();      break;
    case MemoryRequest::LOAD_B:             processLoadByte();          break;
    case MemoryRequest::STORE_W:            processStoreWord();         break;
    case MemoryRequest::STORE_HW:           processStoreHalfWord();     break;
    case MemoryRequest::STORE_B:            processStoreByte();         break;
    case MemoryRequest::IPK_READ:           processIPKRead();           break;

    case MemoryRequest::FETCH_LINE:         processFetchLine();         break;
    case MemoryRequest::STORE_LINE:         processStoreLine();         break;
    case MemoryRequest::FLUSH_LINE:         processFlushLine();         break;
    case MemoryRequest::MEMSET_LINE:        processMemsetLine();        break;
    case MemoryRequest::INVALIDATE_LINE:    processInvalidateLine();    break;
    case MemoryRequest::VALIDATE_LINE:      processValidateLine();      break;

    case MemoryRequest::LOAD_LINKED:        processLoadLinked();        break;
    case MemoryRequest::STORE_CONDITIONAL:  processStoreConditional();  break;
    case MemoryRequest::LOAD_AND_ADD:       processLoadAndAdd();        break;
    case MemoryRequest::LOAD_AND_OR:        processLoadAndOr();         break;
    case MemoryRequest::LOAD_AND_AND:       processLoadAndAnd();        break;
    case MemoryRequest::LOAD_AND_XOR:       processLoadAndXor();        break;
    case MemoryRequest::EXCHANGE:           processExchange();          break;

    default:
      cout << this->name() << " processing invalid memory request type (" << MemoryRequest::name(mActiveRequest.getMemoryMetadata().opcode) << ")" << endl;
      assert(false);
      break;
  }

  // Prepare for the next request if this one has finished.
  if (mActiveRequestComplete)
    startNewRequest();

}

// A handler for each possible operation.

void MemoryBank::processLoadWord() {
  uint32_t data;
  bool cacheHit = currentMemoryHandler().readWord(mActiveData.Address, data, false, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  processLoadResult(cacheHit, data, true);
}

void MemoryBank::processLoadHalfWord() {
  uint32_t data;
  bool cacheHit = currentMemoryHandler().readHalfWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  processLoadResult(cacheHit, data, true);
}

void MemoryBank::processLoadByte() {
  uint32_t data;
  bool cacheHit = currentMemoryHandler().readByte(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  processLoadResult(cacheHit, data, true);
}

void MemoryBank::processStoreWord() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  bool cacheHit = currentMemoryHandler().writeWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  processStoreResult(cacheHit, data);
}

void MemoryBank::processStoreHalfWord() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  bool cacheHit = currentMemoryHandler().writeHalfWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  processStoreResult(cacheHit, data);
}

void MemoryBank::processStoreByte() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  bool cacheHit = currentMemoryHandler().writeByte(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  processStoreResult(cacheHit, data);
}

void MemoryBank::processIPKRead() {
  uint32_t data;
  bool cacheHit = currentMemoryHandler().readWord(mActiveData.Address, data, true, mCacheResumeRequest, false, mActiveData.ReturnChannel);

  Instruction inst(data);
  bool endOfIPK = inst.endOfPacket();
  bool endOfNetworkPacket = endOfIPK || endOfCacheLine(mActiveData.Address);

  processLoadResult(cacheHit, data, endOfNetworkPacket);

  if (cacheHit) {
    if (endOfNetworkPacket) {
      if (endOfIPK && Arguments::memoryTrace())
        MemoryTrace::stopIPK(cBankNumber, mActiveData.Address);
      else if (endOfCacheLine(mActiveData.Address) && Arguments::memoryTrace())
        MemoryTrace::splitLineIPK(cBankNumber, mActiveData.Address);
    } else {
      mActiveData.Address += 4;
    }
  }
}

void MemoryBank::processFetchLine() {
  NetworkResponse flit;

  if (mActiveData.FlitsSent == 0) {
    // Make sure the line is in the cache.
    if (!storedLocally(mActiveData.Address)) {
      processCacheMiss();
      return;
    }
    else {
      // Make sure the line is in the cache line buffer.
      if (!sameCacheLine(mCacheLineBufferTag, mActiveData.Address))
        prepareCacheLineBuffer(mActiveData.Address);

      // If the line is going to a memory, send a header flit telling it the
      // address to which the following data should be stored.
      if (mActiveData.Source != REQUEST_CORES)
        flit = NetworkResponse(mActiveData.Address, mActiveData.ReturnChannel, MemoryRequest::STORE_LINE, false);
      else
        return; // No flit to send this cycle.
    }
  }
  else {
    uint32_t data = readFromCacheLineBuffer();
    bool endOfPacket = (mCacheLineCursor == CACHE_LINE_WORDS);
    flit = assembleResponse(data, endOfPacket);
  }

  assert(canSend());
  send(flit);
}

void MemoryBank::processStoreLine() {
  // Make sure we know where in the cache to store the data.
  // FIXME: this could be called twice. Also, something like this should
  // be used right at the beginning of every operation.
  if (mCacheLineCursor == 0)
    makeSpaceForLine();

  if (mCacheLineCursor == CACHE_LINE_WORDS)
    replaceCacheLine();
  else if (inputAvailable()) {
    uint32_t data = consumeInput().payload().toUInt();
    writeToCacheLineBuffer(data);
  }
}

void MemoryBank::processFlushLine() {
  NetworkRequest flit;

  // First iteration - send header flit.
  if (mActiveData.FlitsSent == 0) {
    // If we don't hold the cache line, it has already been flushed naturally,
    // so there is nothing left to do.
    if (!storedLocally(mActiveData.Address)) {
      mActiveRequestComplete = true;
      return;
    }

    if (DEBUG)
      cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes to 0x" << std::hex << mActiveData.Address << std::dec << endl;

    flit = NetworkRequest(mActiveData.Address, id, MemoryRequest::STORE_LINE, false);
  }
  // Every other iteration - send data.
  else {
    uint32_t data = readFromCacheLineBuffer();
    bool endOfPacket = (mCacheLineCursor == CACHE_LINE_WORDS);

    flit = NetworkRequest(data, id, MemoryRequest::PAYLOAD_ONLY, endOfPacket);
  }

  assert(!mOutputReqQueue.full());
  mOutputReqQueue.write(flit);
  mActiveData.FlitsSent++;
}

void MemoryBank::processMemsetLine() {
  // Make sure we know where in the cache to store the data.
  // FIXME: this could be called twice. Also, something like this should
  // be used right at the beginning of every operation.
  if (mCacheLineCursor == 0)
    makeSpaceForLine();

  if (!inputAvailable())
    return;

  uint32_t data = consumeInput().payload().toUInt();

  // Set the whole cache line in one go.
  for (int i=0; i<CACHE_LINE_WORDS; i++)
    writeToCacheLineBuffer(data);

  replaceCacheLine();
}

void MemoryBank::processInvalidateLine() {assert(false);}
void MemoryBank::processValidateLine() {assert(false);}

void MemoryBank::processLoadLinked() {
  uint32_t data;
  bool cacheHit = currentMemoryHandler().readWord(mActiveData.Address, data, false, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  if (cacheHit)
    mReservations.makeReservation(mActiveData.ReturnChannel.component, mActiveData.Address);
  processLoadResult(cacheHit, data, true);
}

void MemoryBank::processStoreConditional() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  bool cacheHit;

  bool reservationValid = mReservations.checkReservation(mActiveData.ReturnChannel.component, mActiveData.Address);
  if (reservationValid)
    cacheHit = currentMemoryHandler().writeWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  else
    cacheHit = true;  // The operation has finished; continue with the next one.

  // Return whether the store was successful.
  if (cacheHit) {
    NetworkResponse output = assembleResponse(reservationValid, true);
    mOutputQueue.write(output);
  }

  processStoreResult(cacheHit, data);
}

void MemoryBank::processLoadAndAdd() {
  // First half: read data.
  if (!mRMWDataValid)
    processRMWPhase1();
  // Second half: modify and write data.
  else {
    if (!inputAvailable())
      return;

    uint32_t data = getDataToStore();
    data += mRMWData;
    processRMWPhase2(data);
  }
}

void MemoryBank::processLoadAndOr() {
  // First half: read data.
  if (!mRMWDataValid)
    processRMWPhase1();
  // Second half: modify and write data.
  else {
    if (!inputAvailable())
      return;

    uint32_t data = getDataToStore();
    data |= mRMWData;
    processRMWPhase2(data);
  }
}

void MemoryBank::processLoadAndAnd() {
  // First half: read data.
  if (!mRMWDataValid)
    processRMWPhase1();
  // Second half: modify and write data.
  else {
    if (!inputAvailable())
      return;

    uint32_t data = getDataToStore();
    data &= mRMWData;
    processRMWPhase2(data);
  }
}

void MemoryBank::processLoadAndXor() {
  // First half: read data.
  if (!mRMWDataValid)
    processRMWPhase1();
  // Second half: modify and write data.
  else {
    if (!inputAvailable())
      return;

    uint32_t data = getDataToStore();
    data ^= mRMWData;
    processRMWPhase2(data);
  }
}

void MemoryBank::processExchange() {
  // First half: read data.
  if (!mRMWDataValid)
    processRMWPhase1();
  // Second half: write data.
  else {
    if (!inputAvailable())
      return;

    uint32_t data = getDataToStore();
    processRMWPhase2(data);
  }
}

void MemoryBank::processLoadResult(bool cacheHit, uint32_t data, bool endOfPacket) {
  assert(!(mCacheResumeRequest && !cacheHit));
  mCacheResumeRequest = false;
  mActiveRequestComplete = cacheHit && endOfPacket;

  if (cacheHit) {
    if (DEBUG)
      printOperation(mActiveRequest.getMemoryMetadata().opcode,
                     mActiveData.Address, data);

    // Enqueue output request
    NetworkResponse output = assembleResponse(data, endOfPacket);
    assert(canSend());
    send(output);

    // Keep data registered in case it is needed by a read-modify-write op.
    mRMWData = data;
  }
  else
    processCacheMiss();
}

void MemoryBank::processStoreResult(bool cacheHit, uint32_t data) {
  assert(!(mCacheResumeRequest && !cacheHit));
  mCacheResumeRequest = false;
  mActiveRequestComplete = cacheHit;

  if (cacheHit) {
    // Invalidate any atomic transactions relying on the overwritten data.
    mReservations.clearReservation(mActiveData.Address);

    // Dequeue the payload now that we are finished with it.
    consumeInput();

    if (DEBUG)
      printOperation(mActiveRequest.getMemoryMetadata().opcode,
                     mActiveData.Address, data);
  }
  else
    processCacheMiss();
}

uint32_t MemoryBank::getDataToStore() {
  // Only peek, in case we have a cache miss and need to try again.
  assert(inputAvailable());
  NetworkRequest flit = peekInput();

  // The received word should be the continuation of the previous operation.
  if (flit.getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY) {
    cout << "!!!! " << mActiveRequest.getMemoryMetadata().opcode << " | " << mActiveData.Address << " | " << flit.getMemoryMetadata().opcode << " | " << flit.payload() << endl;
    assert(false);
  }

  return flit.payload().toUInt();
}

void MemoryBank::prepareCacheLineBuffer(MemoryAddr address) {
  assert(storedLocally(address));
  mCacheLineBufferTag = getTag(address);
  mGeneralPurposeCacheHandler.fillCacheLineBuffer(mCacheLineBufferTag, mCacheLineBuffer);
  mCacheLineCursor = 0;
}

uint32_t MemoryBank::readFromCacheLineBuffer() {
  uint32_t data = mCacheLineBuffer[mCacheLineCursor];
  MemoryAddr address = mActiveData.Address + mCacheLineCursor*BYTES_PER_WORD;
  assert(sameCacheLine(address, mCacheLineBufferTag));

  if (ENERGY_TRACE && inL1Mode())
    Instrumentation::MemoryBank::readBurstWord(cBankNumber, address, false, mActiveData.ReturnChannel);

  if (DEBUG)
    printOperation(mActiveRequest.getMemoryMetadata().opcode, address, data);

  mCacheLineCursor++;
  assert(mCacheLineCursor <= CACHE_LINE_WORDS);
  bool endOfPacket = (mCacheLineCursor == CACHE_LINE_WORDS);
  mActiveRequestComplete = endOfPacket;

  return data;
}

void MemoryBank::writeToCacheLineBuffer(uint32_t data) {
  MemoryAddr address = mActiveData.Address + mCacheLineCursor*BYTES_PER_WORD;
  assert(sameCacheLine(address, mCacheLineBufferTag));

  if (ENERGY_TRACE && inL1Mode())
    Instrumentation::MemoryBank::writeBurstWord(cBankNumber, address, false, mActiveData.ReturnChannel);

  mCacheLineBuffer[mCacheLineCursor] = data;
  mCacheLineCursor++;
  assert(mCacheLineCursor <= CACHE_LINE_WORDS);

  // Don't check whether this is the end of the operation - we need to call
  // replaceCacheLine first to copy this data to the cache.

  if (DEBUG)
    printOperation(mActiveRequest.getMemoryMetadata().opcode, address, data);
}

void MemoryBank::replaceCacheLine() {
  mGeneralPurposeCacheHandler.replaceCacheLine(mCacheLineBufferTag, mCacheLineBuffer);
  mCacheLineCursor = 0;

  // Invalidate any atomic transactions relying on the overwritten data.
  mReservations.clearReservationRange(mCacheLineBufferTag, mCacheLineBufferTag+mConfig.LineSize);

  // If this cache line contains data required by the callback request,
  // switch to the callback request.
  if (mFSMCallbackState != STATE_IDLE && sameCacheLine(mCacheLineBufferTag, mCallbackData.Address)) {
    mCacheResumeRequest = true;
    mFSMState = mFSMCallbackState;
    mActiveRequest = mCallbackRequest;
    mActiveData = mCallbackData;
    mFSMCallbackState = STATE_IDLE;

    if (DEBUG)
      cout << this->name() << " resuming " << MemoryRequest::name(mActiveRequest.getMemoryMetadata().opcode) << " request" << endl;
  }
  else
    mActiveRequestComplete = true;
}

void MemoryBank::processCacheMiss(bool flushOnly) {
  if (DEBUG)
    cout << this->name() << " cache miss at address 0x" << std::hex << mActiveData.Address << std::dec << endl;

  // Put the current request aside until its data arrives.
  assert(mFSMCallbackState == STATE_IDLE);
  mFSMCallbackState = mFSMState;
  mCallbackRequest = mActiveRequest;
  mCallbackData = mActiveData;
  mFSMState = STATE_IDLE;

  // Prepare a victim cache line.
  mGeneralPurposeCacheHandler.prepareCacheLine(mActiveData.Address, mWriteBackAddress, mWriteBackCount, mCacheLineBuffer, mFetchAddress, mFetchCount);
  mCacheLineCursor = 0;

  if (!flushOnly) {
    // Send read request now.
    MemoryAddr fetchAddress = getTag(mActiveData.Address);
    if (DEBUG)
      cout << this->name() << " buffering request for " << mConfig.LineSize << " bytes from 0x" << std::hex << fetchAddress << std::dec << endl;

    NetworkRequest readRequest(fetchAddress, id, MemoryRequest::FETCH_LINE, true);
    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(readRequest);
  }

  // If data needs to be flushed, start a new flush operation.
  if (mWriteBackCount > 0) {
    mFSMState = STATE_CACHE_FLUSH;
    mCacheLineBufferTag = mWriteBackAddress;
  }

  mActiveRequestComplete = (mWriteBackCount == 0);
}

void MemoryBank::makeSpaceForLine() {
  bool requested = (mFSMCallbackState != STATE_IDLE) &&
        sameCacheLine(mCallbackData.Address, mActiveData.Address);
  if (!requested && !storedLocally(mActiveData.Address))
    processCacheMiss(true);   // true = don't bother fetching the new line
  else
    mGeneralPurposeCacheHandler.findCacheLine(mActiveData.Address);
}

bool MemoryBank::inputAvailable() const {
  switch (mActiveData.Source) {
    case REQUEST_CORES:
      return !mInputQueue.empty() && !onlyAcceptingRefills();
    case REQUEST_MEMORIES:
      return iRequest.valid() && !onlyAcceptingRefills();
    case REQUEST_REFILL:
      return iResponse.valid() && iResponseTarget.read() == id.position-CORES_PER_TILE;
    case REQUEST_NONE:
      bool newCoreRequest = !mInputQueue.empty();
      bool newMemoryRequest = iRequest.valid() && iRequest.read().getMemoryMetadata().opcode != MemoryRequest::PAYLOAD_ONLY;
      bool newRefillRequest = iResponse.valid() && iResponseTarget.read() == id.position-CORES_PER_TILE;
      return (!onlyAcceptingRefills() && (newCoreRequest || newMemoryRequest)) ||
          newRefillRequest;
  }

  assert(false);
  return false;
}

const NetworkRequest MemoryBank::peekInput() {
  switch (mActiveData.Source) {
    case REQUEST_CORES:
      assert(!mInputQueue.empty());
      return mInputQueue.peek();
    case REQUEST_MEMORIES:
      assert(iRequest.valid());
      return iRequest.read();
    case REQUEST_REFILL:
      assert(iResponse.valid());
      return iResponse.read();
    case REQUEST_NONE:
      // Prioritise refills since they may be holding up other requests.
      if (iResponse.valid() && iResponseTarget.read() == id.position-CORES_PER_TILE) {
        mActiveData.Source = REQUEST_REFILL;
        return iResponse.read();
      }
      else if (!mInputQueue.empty()) {
        mActiveData.Source = REQUEST_CORES;
        return mInputQueue.peek();
      }
      else {
        assert(iRequest.valid());
        mActiveData.Source = REQUEST_MEMORIES;
        return iRequest.read();
      }
  }

  assert(false);
  return NetworkRequest();
}

const NetworkRequest MemoryBank::consumeInput() {
  NetworkRequest request;

  switch (mActiveData.Source) {
    case REQUEST_CORES:
      assert(!mInputQueue.empty());
      request = mInputQueue.read();
      if (DEBUG)
        cout << this->name() << " dequeued " << request << endl;
      break;
    case REQUEST_MEMORIES:
      assert(iRequest.valid());
      request = iRequest.read();
      iRequest.ack();
      break;
    case REQUEST_REFILL:
      assert(iResponse.valid());
      request = iResponse.read();
      iResponse.ack();
      break;
    case REQUEST_NONE:
      // Prioritise refills since they may be holding up other requests.
      if (iResponse.valid() && iResponseTarget.read() == id.position-CORES_PER_TILE) {
        mActiveData.Source = REQUEST_REFILL;
        request = iResponse.read();
        iResponse.ack();
      }
      else if (!mInputQueue.empty()) {
        mActiveData.Source = REQUEST_CORES;
        request = mInputQueue.read();
        if (DEBUG)
          cout << this->name() << " dequeued " << request << endl;
      }
      else {
        assert(iRequest.valid());
        mActiveData.Source = REQUEST_MEMORIES;
        request = iRequest.read();
        iRequest.ack();
      }
      break;
  }

  return request;
}

bool MemoryBank::canSend() const {
  switch (mActiveData.Source) {
    case REQUEST_CORES:
      return !mOutputQueue.full();
    case REQUEST_MEMORIES:
      return !oResponse.valid();
    case REQUEST_REFILL:
      return true;  // Refills never need to send any data.
    default:
      // If we don't know where the request came from, we can't know where to
      // send the response.
      cerr << "Error: mActiveData.Source " << mActiveData.Source << " is trying to send data"<< endl;
      assert(false);
      return false;
  }
}

void MemoryBank::send(const NetworkData& data) {
  switch (mActiveData.Source) {
    case REQUEST_CORES:
      mOutputQueue.write(data);
      break;
    case REQUEST_MEMORIES:
      oResponse.write(data);
      break;
    default:
      // If we don't know where the request came from, we can't know where to
      // send the response.
      assert(false);
      break;
  }
  mActiveData.FlitsSent++;
}

void MemoryBank::processRMWPhase1() {
  // Load data as usual...
  processLoadWord();

  // ... except the operation hasn't finished yet.
  mActiveRequestComplete = false;

  // If data is in the cache, signal that we are ready for phase 2.
  if (mFSMState == STATE_LOCAL_MEMORY_ACCESS)
    mRMWDataValid = true;
}

void MemoryBank::processRMWPhase2(uint32_t data) {
  // A cache hit is guaranteed because there cannot have been any intervening
  // memory operations since the data was loaded.
  bool cacheHit = currentMemoryHandler().writeWord(mActiveData.Address, data, mCacheResumeRequest, false, mActiveData.ReturnChannel);
  assert(cacheHit);

  // Finished phase 2.
  mRMWDataValid = false;

  processStoreResult(cacheHit, data);
  mActiveRequestComplete = cacheHit;
}

void MemoryBank::processDelayedCacheFlush() {
  // Start a new flush request.
  NetworkRequest header(mWriteBackAddress, id, MemoryRequest::FLUSH_LINE, true);
  processMessageHeader(header);
}

void MemoryBank::processValidInput() {
  if (DEBUG) cout << this->name() << " received " << iData.read() << endl;
  assert(iData.valid());
  assert(!mInputQueue.full());

  mInputQueue.write(iData.read());
  iData.ack();
}

NetworkResponse MemoryBank::assembleResponse(uint32_t data, bool endOfPacket) {
  return NetworkResponse(data, mActiveData.ReturnChannel, MemoryRequest::PAYLOAD_ONLY, endOfPacket);
}

void MemoryBank::handleDataOutput() {
  // If we have new data to send:
  if (!mOutputWordPending) {
    if (mOutputQueue.empty()) {
      next_trigger(mOutputQueue.writeEvent());
    }
    else {
      mActiveOutputWord = mOutputQueue.read();
      mOutputWordPending = true;

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
  // Memory banks are clocked on the negative edge.
  if (!iClock.negedge()) {
    next_trigger(iClock.negedge_event());
    return;
  }

  // Only allow an operation to begin if we are sure that there is space in
  // the output request queue to hold any potential messages.
  // Decode the request and get into the correct state before performing the
  // operation.
  if (mFSMState == STATE_IDLE && mOutputReqQueue.empty())
    startNewRequest();

  // Act on current state.
  switch (mFSMState) {
    case STATE_IDLE:                                                   break;
    case STATE_LOCAL_MEMORY_ACCESS:  processLocalMemoryAccess();       break;
    case STATE_CACHE_FLUSH:          processDelayedCacheFlush();       break;
  }

  updateIdle();

  if (mFSMState != STATE_IDLE)
    next_trigger(iClock.negedge_event());
  // Else wait for any input to arrive (default sensitivity).
}

void MemoryBank::updateIdle() {
  bool wasIdle = currentlyIdle;
  currentlyIdle = mFSMState == STATE_IDLE &&
                  mInputQueue.empty() && mOutputQueue.empty() &&
                  mOutputReqQueue.empty() &&
                  !mOutputWordPending;

  if (wasIdle != currentlyIdle)
    Instrumentation::idle(id, currentlyIdle);
}

void MemoryBank::updateReady() {
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
  mInputQueue("mInputQueue", IN_CHANNEL_BUFFER_SIZE),
  mOutputQueue("mOutputQueue", OUT_CHANNEL_BUFFER_SIZE, INTERNAL_LATENCY),
  mOutputReqQueue("mOutputReqQueue", 10 /*read addr + write addr + cache line*/, 0),
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
	mActiveRequestComplete = true;
	mActiveData.Address = -1;
	mActiveData.Source = REQUEST_NONE;
	mActiveData.ReturnChannel = ChannelID();

	mRMWData = 0;
	mRMWDataValid = false;

	//-- Cache mode state -------------------------------------------------------------------------

	mCacheResumeRequest = false;
	mCacheLineCursor = 0;
	mCacheLineBufferTag = 0;
	mWriteBackAddress = 0;
	mWriteBackCount = 0;
	mFetchAddress = 0;
	mFetchCount = 0;

	//-- L2 cache mode state ----------------------------------------------------------------------

	mRequestState = REQ_READY;
	mWaitingForL2Consensus = false;

	//-- Debug utilities --------------------------------------------------------------------------

	mBackgroundMemory = 0;
	localNetwork = 0;

	//-- Port initialization ----------------------------------------------------------------------

	oReadyForData.initialize(false);

	//-- Register module with SystemC simulation kernel -------------------------------------------

	currentlyIdle = true;
	Instrumentation::idle(id, true);

	SC_METHOD(mainLoop);
	sensitive << iRequest << iResponse << mInputQueue.writeEvent();
	dont_initialize();

	SC_METHOD(updateReady);
	sensitive << mInputQueue.readEvent() << mInputQueue.writeEvent();
	// do initialise

	SC_METHOD(processValidInput);
	sensitive << iData;
	dont_initialize();

	SC_METHOD(handleDataOutput);

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

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(mBackgroundMemory != NULL);

	size_t count = data.size();
	uint32_t address = location;

	assert(address % 4 == 0);
	assert(mConfig.Mode != MODE_INACTIVE);

  for (size_t i = 0; i < count; i++) {
    if (!currentMemoryHandler().writeWord(address + i * 4, data[i].toUInt(), false, true, ChannelID()))
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

  while (address < limit) {
    uint32_t data;
    bool cached = currentMemoryHandler().readWord(address, data, false, false, true, ChannelID());

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

  uint32_t data;
  bool cached = currentMemoryHandler().readWord(addr, data, false, false, true, ChannelID());

  if (!cached)
    data = mBackgroundMemory->readWord(addr).toUInt();

  return Word(data);
}

Word MemoryBank::readByte(MemoryAddr addr) {
	assert(mBackgroundMemory != NULL);

  uint32_t data;
  bool cached = currentMemoryHandler().readByte(addr, data, false, true, ChannelID());

  if (!cached)
    data = mBackgroundMemory->readByte(addr).toUInt();

  return Word(data);
}

void MemoryBank::writeWord(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);
	assert(addr % 4 == 0);

  bool cached = currentMemoryHandler().writeWord(addr, data.toUInt(), false, true, ChannelID());

  if (!cached)
    mBackgroundMemory->writeWord(addr, data.toUInt());
}

void MemoryBank::writeByte(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);

  bool cached = currentMemoryHandler().writeByte(addr, data.toUInt(), false, true, ChannelID());

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
  cout << this->name() << ": " << MemoryRequest::name(operation) <<
      ", address = 0x" << std::hex << address << ", data = 0x" << data << std::dec;

  if (operation == MemoryRequest::IPK_READ) {
    Instruction inst(data);
    cout << " (" << inst << ")";
  }

  cout << endl;
}
