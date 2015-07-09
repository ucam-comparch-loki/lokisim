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
#include "SimplifiedOnChipScratchpad.h"
#include "../../Datatype/Instruction.h"
#include "../../Network/Topologies/LocalNetwork.h"
#include "../../Utility/Arguments.h"
#include "../../Utility/Instrumentation/MemoryBank.h"
#include "../../Utility/Instrumentation/Network.h"
#include "../../Utility/Trace/MemoryTrace.h"
#include "../../Utility/Parameters.h"
#include "../../Exceptions/ReadOnlyException.h"
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
    return !mActiveData.Request.getMemoryMetadata().scratchpadL1;
  else
    return !mActiveData.Request.getMemoryMetadata().scratchpadL2;
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
  return /*!HIT_UNDER_MISS &&*/ (mCallbackData.State != STATE_IDLE);
}

MemoryAddr MemoryBank::getTag(MemoryAddr address) const {
  // Cache line = 32 bytes = 2^5 bytes.
  return (address & ~0x1F);
}

MemoryAddr MemoryBank::getOffset(MemoryAddr address) const {
  // Cache line = 32 bytes = 2^5 bytes.
  return (address & 0x1F);
}

bool MemoryBank::sameCacheLine(MemoryAddr address1, MemoryAddr address2) const {
  return getTag(address1) == getTag(address2);
}

bool MemoryBank::endOfCacheLine(MemoryAddr address) const {
  return !sameCacheLine(address, address+4);
}

bool MemoryBank::storedLocally(MemoryAddr address) const {
  if (checkTags())
    return mGeneralPurposeCacheHandler.lookupCacheLine(address).Hit;
  else
    return true;
}

bool MemoryBank::startNewRequest() {
  assert(mActiveData.Complete);
  mActiveData.State = STATE_IDLE;
  mActiveData.Source = REQUEST_NONE;

  if (!inputAvailable())
		return false;

  // TODO: if allowing hit-under-miss, do a tag look-up to determine if hit or not.

  NetworkRequest request = consumeInput();
  return processMessageHeader(request);
}

bool MemoryBank::processMessageHeader(const NetworkRequest& request) {
  MemoryOperation opcode = request.getMemoryMetadata().opcode;

  mActiveData.State = STATE_LOCAL_MEMORY_ACCESS;
  mActiveData.Address = request.payload().toUInt();
	mActiveData.ReturnChannel = returnChannel(request);
	mActiveData.FlitsSent = 0;
	mActiveData.Request = request;
	mActiveData.Complete = false;

  if (DEBUG)
    cout << this->name() << " starting " << memoryOpName(opcode) << " request from component " << mActiveData.ReturnChannel.component << endl;

  // If the operation is going to access memory, make sure the necessary cache
  // line is stored locally, or there is a space for it to be fetched into.
	switch (opcode) {
	case UPDATE_DIRECTORY_ENTRY:
	case UPDATE_DIRECTORY_MASK:
	  // Forward the request on to the tile's directory.
    if (DEBUG)
      cout << this->name() << " buffering directory update request: " << mActiveData.Request.payload() << endl;

    assert(!mOutputReqQueue.full());
    mOutputReqQueue.write(mActiveData.Request);
    mActiveData.State = STATE_IDLE;
    mActiveData.Complete = true;
    break;

	case IPK_READ:
		if (ENERGY_TRACE)
		  Instrumentation::MemoryBank::initiateIPKRead(cBankNumber);
    getCacheLine(mActiveData.Address, false, true, true, true);
		break;

	case FETCH_LINE:
    if (ENERGY_TRACE)
      Instrumentation::MemoryBank::initiateBurstRead(cBankNumber);
    getCacheLine(mActiveData.Address, false, true, true, false);
    break;

	case FLUSH_LINE:
    if (ENERGY_TRACE)
      Instrumentation::MemoryBank::initiateBurstRead(cBankNumber);
    getCacheLine(mActiveData.Address, false, false, true, false);
    break;

	case STORE_LINE:
	case MEMSET_LINE:
	  if (ENERGY_TRACE)
	    Instrumentation::MemoryBank::initiateBurstWrite(cBankNumber);
    getCacheLine(mActiveData.Address, true, true, false, false);
    break;

	case LOAD_W:
	case LOAD_HW:
	case LOAD_B:
	case LOAD_LINKED:
    getCacheLine(mActiveData.Address, false, true, true, false);
    break;

	case LOAD_AND_ADD:
	case LOAD_AND_AND:
	case LOAD_AND_OR:
	case LOAD_AND_XOR:
	case EXCHANGE:
	  if (mBackgroundMemory->readOnly(mActiveData.Address))
	    throw ReadOnlyException(mActiveData.Address);
	  getCacheLine(mActiveData.Address, false, true, true, false);
	  break;

	case STORE_W:
	case STORE_HW:
	case STORE_B:
	case STORE_CONDITIONAL:
	  if (mBackgroundMemory->readOnly(mActiveData.Address))
	    throw ReadOnlyException(mActiveData.Address);
    getCacheLine(mActiveData.Address, false, true, false, false);
		break;

  default:
    throw InvalidOptionException("memory bank message header opcode", opcode);
    break;
	}

	return true;
}

void MemoryBank::getCacheLine(MemoryAddr address, bool validate, bool required, bool isRead, bool isInstruction) {
  MemoryAddr tag = getTag(address);
  CacheLookup info = mGeneralPurposeCacheHandler.prepareCacheLine(address, mCacheLineBuffer, isRead, isInstruction);

  Instrumentation::MemoryBank::startOperation(cBankNumber,
                                              mActiveData.Request.getMemoryMetadata().opcode,
                                              address,
                                              !info.Hit,
                                              mActiveData.ReturnChannel);

  // If we don't need to fetch, just set the tag and wait for data.
  if (validate)
    mCacheLineBuffer.setTag(tag);

  // Check whether we have already requested data from this line, and know
  // where it will go in the cache.
  if (mCallbackData.State != STATE_IDLE &&
      sameCacheLine(mCallbackData.Address, mActiveData.Address)) {
    mActiveData.Position = getTag(mCallbackData.Position) + getOffset(address);
    return;
  }
  else
    mActiveData.Position = info.SRAMLine*CACHE_LINE_WORDS*BYTES_PER_WORD + getOffset(address);

  // We only have to do something if we don't already have the data.
  if (!info.Hit) {
    // Bail out if the operation doesn't need to happen when the data isn't
    // present (e.g. FLUSH_LINE).
    if (!required) {
      mActiveData.Complete = true;
      return;
    }

    // If we need to flush or fetch a line, put aside the current operation.
    if (!validate || info.FlushRequired) {
      if (DEBUG)
        cout << this->name() << " cache miss at address 0x" << std::hex << mActiveData.Address << std::dec << endl;

      // Put the current request aside until its data arrives.
      assert(mCallbackData.State == STATE_IDLE);
      mCallbackData = mActiveData;
      mActiveData.State = STATE_IDLE;
    }

    // Fetch the line, if necessary.
    if (!validate) {
      if (DEBUG)
        cout << this->name() << " buffering request for cache line from 0x" << std::hex << tag << std::dec << endl;

      NetworkRequest readRequest(tag, id, FETCH_LINE, true);
      assert(!mOutputReqQueue.full());
      mOutputReqQueue.write(readRequest);
    }

    // Flush the victim line, if necessary.
    if (info.FlushRequired) {
      mActiveData.State = STATE_CACHE_FLUSH;
      mActiveData.Address = info.FlushAddress;
    }

    mActiveData.Complete = !validate || info.FlushRequired;
  }
}

void MemoryBank::processLocalMemoryAccess() {
	if (!canSend()) {
		if (DEBUG)
			cout << this->name() << " delayed request due to full output queue" << endl;
		return;
	}

	// If we have got this far, we are sure that the required cache line has been
	// allocated, and that we are able to send any result.

  switch (mActiveData.Request.getMemoryMetadata().opcode) {
    case LOAD_W:             processLoadWord();          break;
    case LOAD_HW:            processLoadHalfWord();      break;
    case LOAD_B:             processLoadByte();          break;
    case STORE_W:            processStoreWord();         break;
    case STORE_HW:           processStoreHalfWord();     break;
    case STORE_B:            processStoreByte();         break;
    case IPK_READ:           processIPKRead();           break;

    case FETCH_LINE:         processFetchLine();         break;
    case STORE_LINE:         processStoreLine();         break;
    case FLUSH_LINE:         processFlushLine();         break;
    case MEMSET_LINE:        processMemsetLine();        break;
    case INVALIDATE_LINE:    processInvalidateLine();    break;
    case VALIDATE_LINE:      processValidateLine();      break;

    case LOAD_LINKED:        processLoadLinked();        break;
    case STORE_CONDITIONAL:  processStoreConditional();  break;
    case LOAD_AND_ADD:       processLoadAndAdd();        break;
    case LOAD_AND_OR:        processLoadAndOr();         break;
    case LOAD_AND_AND:       processLoadAndAnd();        break;
    case LOAD_AND_XOR:       processLoadAndXor();        break;
    case EXCHANGE:           processExchange();          break;

    default:
      cout << this->name() << " processing invalid memory request type (" << memoryOpName(mActiveData.Request.getMemoryMetadata().opcode) << ")" << endl;
      assert(false);
      break;
  }

  // Prepare for the next request if this one has finished.
  if (mActiveData.Complete) {
    mActiveData.State = STATE_IDLE;
    next_trigger(iClock.negedge_event());
  }

}

// A handler for each possible operation.

void MemoryBank::processLoadWord() {
  uint32_t data = currentMemoryHandler().readWord(mActiveData.Position);
  processLoadResult(data, true);
}

void MemoryBank::processLoadHalfWord() {
  uint32_t data = currentMemoryHandler().readHalfWord(mActiveData.Position);
  processLoadResult(data, true);
}

void MemoryBank::processLoadByte() {
  uint32_t data = currentMemoryHandler().readByte(mActiveData.Position);
  processLoadResult(data, true);
}

void MemoryBank::processStoreWord() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  currentMemoryHandler().writeWord(mActiveData.Position, data);
  processStoreResult(data);
}

void MemoryBank::processStoreHalfWord() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  currentMemoryHandler().writeHalfWord(mActiveData.Position, data);
  processStoreResult(data);
}

void MemoryBank::processStoreByte() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();
  currentMemoryHandler().writeByte(mActiveData.Position, data);
  processStoreResult(data);
}

void MemoryBank::processIPKRead() {
  uint32_t data = currentMemoryHandler().readWord(mActiveData.Position);

  // The first word will have been dealt with when we started the operation.
  if (mActiveData.FlitsSent > 0)
    Instrumentation::MemoryBank::readIPKWord(cBankNumber, mActiveData.Address, false, mActiveData.ReturnChannel);

  Instruction inst(data);
  bool endOfIPK = inst.endOfPacket();
  bool endOfNetworkPacket = endOfIPK || endOfCacheLine(mActiveData.Address);

  processLoadResult(data, endOfNetworkPacket);

  if (endOfNetworkPacket) {
    if (endOfIPK && Arguments::memoryTrace())
      MemoryTrace::stopIPK(cBankNumber, mActiveData.Address);
    else if (endOfCacheLine(mActiveData.Address) && Arguments::memoryTrace())
      MemoryTrace::splitLineIPK(cBankNumber, mActiveData.Address);
  } else {
    mActiveData.Address += 4;
    mActiveData.Position += 4;
  }
}

void MemoryBank::processFetchLine() {
  NetworkResponse flit;

  if (mActiveData.FlitsSent == 0) {
    // Make sure the line is in the cache line buffer.
    if (!mCacheLineBuffer.inBuffer(mActiveData.Address))
      prepareCacheLineBuffer(mActiveData.Address);

    // If the line is going to a memory, send a header flit telling it the
    // address to which the following data should be stored.
    if (mActiveData.Source != REQUEST_CORES)
      flit = NetworkResponse(mActiveData.Address, mActiveData.ReturnChannel, STORE_LINE, false);
    else
      return; // No flit to send this cycle.
  }
  else {
    uint32_t data = readFromCacheLineBuffer();
    bool endOfPacket = mCacheLineBuffer.finishedOperation();
    flit = assembleResponse(data, endOfPacket);
  }

  assert(canSend());
  send(flit);
}

void MemoryBank::processStoreLine() {
  if (mCacheLineBuffer.finishedOperation())
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
    if (DEBUG)
      cout << this->name() << " buffering request for cache line to 0x" << std::hex << mActiveData.Address << std::dec << endl;

    flit = NetworkRequest(mActiveData.Address, id, STORE_LINE, false);
  }
  // Every other iteration - send data.
  else {
    uint32_t data = readFromCacheLineBuffer();
    bool endOfPacket = mCacheLineBuffer.finishedOperation();

    if (DEBUG)
      cout << this->name() << " buffering request data: " << data << endl;

    flit = NetworkRequest(data, id, PAYLOAD, endOfPacket);
  }

  assert(!mOutputReqQueue.full());
  mOutputReqQueue.write(flit);
  mActiveData.FlitsSent++;
}

void MemoryBank::processMemsetLine() {
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
  uint32_t data = currentMemoryHandler().readWord(mActiveData.Position);
  mReservations.makeReservation(mActiveData.ReturnChannel.component, mActiveData.Address);
  processLoadResult(data, true);
}

void MemoryBank::processStoreConditional() {
  if (!inputAvailable())
    return;

  uint32_t data = getDataToStore();

  bool reservationValid = mReservations.checkReservation(mActiveData.ReturnChannel.component, mActiveData.Address);
  if (reservationValid)
    currentMemoryHandler().writeWord(mActiveData.Position, data);

  NetworkResponse output = assembleResponse(reservationValid, true);
  mOutputQueue.write(output);

  processStoreResult(data);
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

void MemoryBank::processLoadResult(uint32_t data, bool endOfPacket) {
  mActiveData.Complete = endOfPacket;

  if (DEBUG)
    printOperation(mActiveData.Request.getMemoryMetadata().opcode,
                   mActiveData.Address, data);

  // Enqueue output request
  NetworkResponse output = assembleResponse(data, endOfPacket);
  assert(canSend());
  send(output);

  // Keep data registered in case it is needed by a read-modify-write op.
  mRMWData = data;
}

void MemoryBank::processStoreResult(uint32_t data) {
  mActiveData.Complete = true;

  // Invalidate any atomic transactions relying on the overwritten data.
  mReservations.clearReservation(mActiveData.Address);

  // Dequeue the payload now that we are finished with it.
  consumeInput();

  if (DEBUG)
    printOperation(mActiveData.Request.getMemoryMetadata().opcode,
                   mActiveData.Address, data);
}

uint32_t MemoryBank::getDataToStore() {
  // Only peek, in case we have a cache miss and need to try again.
  assert(inputAvailable());
  NetworkRequest flit = peekInput();

  // The received word should be the continuation of the previous operation.
  if (flit.getMemoryMetadata().opcode != PAYLOAD && flit.getMemoryMetadata().opcode != PAYLOAD_EOP) {
    cout << "!!!! " << mActiveData.Request.getMemoryMetadata().opcode << " | " << mActiveData.Address << " | " << flit.getMemoryMetadata().opcode << " | " << flit.payload() << endl;
    assert(false);
  }

  return flit.payload().toUInt();
}

void MemoryBank::prepareCacheLineBuffer(MemoryAddr address) {
  mGeneralPurposeCacheHandler.fillCacheLineBuffer(getTag(address), mCacheLineBuffer);
}

uint32_t MemoryBank::readFromCacheLineBuffer() {
  MemoryAddr address = mCacheLineBuffer.getCurrentAddress();
  assert(sameCacheLine(address, mActiveData.Address));

  uint32_t data = mCacheLineBuffer.read();

  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::readBurstWord(cBankNumber, address, false, mActiveData.ReturnChannel);

  if (DEBUG)
    printOperation(mActiveData.Request.getMemoryMetadata().opcode, address, data);

  mActiveData.Complete = mCacheLineBuffer.finishedOperation();

  return data;
}

void MemoryBank::writeToCacheLineBuffer(uint32_t data) {
  MemoryAddr address = mCacheLineBuffer.getCurrentAddress();
  assert(sameCacheLine(address, mActiveData.Address));

  if (ENERGY_TRACE)
    Instrumentation::MemoryBank::writeBurstWord(cBankNumber, address, false, mActiveData.ReturnChannel);

  mCacheLineBuffer.write(data);

  // Don't check whether this is the end of the operation - we need to call
  // replaceCacheLine first to copy this data to the cache.

  if (DEBUG)
    printOperation(mActiveData.Request.getMemoryMetadata().opcode, address, data);
}

void MemoryBank::replaceCacheLine() {
  mGeneralPurposeCacheHandler.replaceCacheLine(mCacheLineBuffer, mActiveData.Position);

  // Invalidate any atomic transactions relying on the overwritten data.
  mReservations.clearReservationRange(mCacheLineBuffer.getTag(), mCacheLineBuffer.getTag()+CACHE_LINE_BYTES);

  // If this cache line contains data required by the callback request,
  // switch to the callback request.
  if (mCallbackData.State != STATE_IDLE && mCacheLineBuffer.inBuffer(mCallbackData.Address)) {
    mActiveData = mCallbackData;
    mCallbackData.State = STATE_IDLE;

    if (DEBUG)
      cout << this->name() << " resuming " << memoryOpName(mActiveData.Request.getMemoryMetadata().opcode) << " request" << endl;
  }
  else
    mActiveData.Complete = true;
}

bool MemoryBank::inputAvailable() const {
  switch (mActiveData.Source) {
    case REQUEST_CORES:
      return !mInputQueue.empty() && !onlyAcceptingRefills();
    case REQUEST_MEMORIES:
      return requestSig.valid() && !onlyAcceptingRefills();
    case REQUEST_REFILL:
      return iResponse.valid() && iResponseTarget.read() == id.position-CORES_PER_TILE;
    case REQUEST_NONE:
      bool newCoreRequest = !mInputQueue.empty();
      bool newMemoryRequest = requestSig.valid();
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
      assert(requestSig.valid());
      return requestSig.read();
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
        mActiveData.Source = REQUEST_MEMORIES;
        assert(requestSig.valid());
        return requestSig.read();
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
      assert(requestSig.valid());
      request = requestSig.read();
      requestSig.ack();
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
        mActiveData.Source = REQUEST_MEMORIES;
        assert(requestSig.valid());
        request = requestSig.read();
        requestSig.ack();
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
  mActiveData.Complete = false;

  // If data is in the cache, signal that we are ready for phase 2.
  if (mActiveData.State == STATE_LOCAL_MEMORY_ACCESS)
    mRMWDataValid = true;
}

void MemoryBank::processRMWPhase2(uint32_t data) {
  currentMemoryHandler().writeWord(mActiveData.Position, data);

  // Finished phase 2.
  mRMWDataValid = false;

  processStoreResult(data);
  mActiveData.Complete = true;
}

void MemoryBank::processDelayedCacheFlush() {
  // Start a new flush request.
  NetworkRequest header(mActiveData.Address, id, FLUSH_LINE, true);
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
  if (mActiveData.ReturnChannel.isCore())
    return NetworkResponse(data, mActiveData.ReturnChannel, endOfPacket);
  else
    return NetworkResponse(data, mActiveData.ReturnChannel, PAYLOAD, endOfPacket);
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
  if (mActiveData.State == STATE_IDLE && mOutputReqQueue.empty())
    startNewRequest();

  // Act on current state.
  switch (mActiveData.State) {
    case STATE_IDLE:                                                   break;
    case STATE_LOCAL_MEMORY_ACCESS:  processLocalMemoryAccess();       break;
    case STATE_CACHE_FLUSH:          processDelayedCacheFlush();       break;
  }

  updateIdle();

  if (!currentlyIdle)
    next_trigger(iClock.negedge_event());
  // Else wait for any input to arrive (default sensitivity).
}

void MemoryBank::updateIdle() {
  bool wasIdle = currentlyIdle;
  currentlyIdle = mActiveData.State == STATE_IDLE &&
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
  mInputQueue(string(this->name()) + string(".mInputQueue"), IN_CHANNEL_BUFFER_SIZE),
  mOutputQueue("mOutputQueue", OUT_CHANNEL_BUFFER_SIZE, INTERNAL_LATENCY),
  mOutputReqQueue("mOutputReqQueue", 10 /*read addr + write addr + cache line*/, 0),
  mData(MEMORY_BANK_SIZE),
  mReservations(1),
	mScratchpadModeHandler(bankNumber, mData),
	mGeneralPurposeCacheHandler(bankNumber, mData),
	mL2RequestFilter("request_filter", ID, this)
{
	//-- Configuration parameters -----------------------------------------------------------------

	assert(cMemoryBanks > 0);
	assert(cBankNumber < cMemoryBanks);

	//-- Data queue state -------------------------------------------------------------------------

	mOutputWordPending = false;
	mActiveOutputWord = NetworkData();

	//-- Mode independent state -------------------------------------------------------------------

	mActiveData.State = STATE_IDLE;
	mCallbackData.State = STATE_IDLE;

	mActiveData.Request = NetworkRequest();
	mActiveData.Complete = true;
	mActiveData.Address = -1;
	mActiveData.Source = REQUEST_NONE;
	mActiveData.ReturnChannel = ChannelID();

	mRMWData = 0;
	mRMWDataValid = false;

	//-- Debug utilities --------------------------------------------------------------------------

	mBackgroundMemory = 0;
	localNetwork = 0;

	//-- Port initialization ----------------------------------------------------------------------

	oReadyForData.initialize(false);

	mL2RequestFilter.iClock(iClock);
	mL2RequestFilter.iRequest(iRequest);
	mL2RequestFilter.iRequestTarget(iRequestTarget);
	mL2RequestFilter.oRequest(requestSig);
	mL2RequestFilter.iRequestClaimed(iRequestClaimed);
	mL2RequestFilter.oClaimRequest(oClaimRequest);

	//-- Register module with SystemC simulation kernel -------------------------------------------

	currentlyIdle = true;
	Instrumentation::idle(id, true);

	SC_METHOD(mainLoop);
	sensitive << requestSig << iResponse << mInputQueue.writeEvent();
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
}

void MemoryBank::storeData(vector<Word>& data, MemoryAddr location) {
	assert(false);
}

void MemoryBank::synchronizeData() {
	assert(mBackgroundMemory != NULL);
//	if (mConfig.Mode == MODE_GP_CACHE)
//		mGeneralPurposeCacheHandler.synchronizeData(mBackgroundMemory);
	throw UnsupportedFeatureException("memory data synchronisation");
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
    uint32_t data = readWord(address).toUInt();
    cout << "0x" << setprecision(8) << setfill('0') << hex << address << ":  " << "0x" << setprecision(8) << setfill('0') << hex << data << dec << endl;
    address += 4;
  }
}

Word MemoryBank::readWord(MemoryAddr addr) {
	assert(mBackgroundMemory != NULL);
	assert(addr % 4 == 0);

	CacheLookup info = mGeneralPurposeCacheHandler.lookupCacheLine(addr);

  if (info.Hit)
    return mGeneralPurposeCacheHandler.readWord(info.SRAMLine*CACHE_LINE_WORDS*BYTES_PER_WORD + getOffset(addr));
  else
    return mBackgroundMemory->readWord(addr);
}

Word MemoryBank::readByte(MemoryAddr addr) {
	assert(mBackgroundMemory != NULL);

  CacheLookup info = mGeneralPurposeCacheHandler.lookupCacheLine(addr);

  if (info.Hit)
    return mGeneralPurposeCacheHandler.readByte(info.SRAMLine*CACHE_LINE_WORDS*BYTES_PER_WORD + getOffset(addr));
  else
    return mBackgroundMemory->readByte(addr);
}

void MemoryBank::writeWord(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);
	assert(addr % 4 == 0);

  CacheLookup info = mGeneralPurposeCacheHandler.lookupCacheLine(addr);

  if (info.Hit)
    mGeneralPurposeCacheHandler.writeWord(info.SRAMLine*CACHE_LINE_WORDS*BYTES_PER_WORD + getOffset(addr), data.toUInt());
  else
    mBackgroundMemory->writeWord(addr, data.toUInt());
}

void MemoryBank::writeByte(MemoryAddr addr, Word data) {
	assert(mBackgroundMemory != NULL);

  CacheLookup info = mGeneralPurposeCacheHandler.lookupCacheLine(addr);

  if (info.Hit)
    mGeneralPurposeCacheHandler.writeByte(info.SRAMLine*CACHE_LINE_WORDS*BYTES_PER_WORD + getOffset(addr), data.toUInt());
  else
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

void MemoryBank::printOperation(MemoryOperation operation,
                                MemoryAddr                     address,
                                uint32_t                       data) const {
  cout << this->name() << ": " << memoryOpName(operation) <<
      ", address = 0x" << std::hex << address << ", data = 0x" << data << std::dec;

  if (operation == IPK_READ) {
    Instruction inst(data);
    cout << " (" << inst << ")";
  }

  cout << endl;
}
