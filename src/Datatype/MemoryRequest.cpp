/*
 * MemoryRequest.cpp
 *
 * Current layout:
 *    16 bit read/write address
 *    16 bit return address (only need 8?)
 *    8 bit number of operations to do (e.g. request 8 sequential reads at once)
 *    1 read/write bit (1 = read, 0 = write)
 *    1 IPK bit (says whether we want to read a whole instruction packet)
 *    22 bits spare (could include a stride length?)
 *
 *    | spare  | IPK bit | r/w bit | num ops | return address | start address |
 *     63       41        40        39        31               15             0
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#include "MemoryRequest.h"

const short startStartAddr = 0;
const short startRetAddr   = startStartAddr + 16;
const short startNumOps    = startRetAddr + 16;
const short startRWBit     = startNumOps + 8;
const short startIPKBit    = startRWBit + 1;
const short end            = startIPKBit + 1;

/* Return the memory address to read from or write to. */
unsigned short MemoryRequest::getMemoryAddress() const {
  return getBits(startStartAddr, startRetAddr - 1);
}

/* Return the channel ID to send any data back to. */
unsigned short MemoryRequest::getReturnAddress() const {
  return getBits(startRetAddr, startNumOps - 1);
}

/* Return the number of reads/writes the client would like to carry out. */
unsigned short MemoryRequest::getNumOps() const {
  return getBits(startNumOps, startRWBit - 1);
}

/* Return whether or not the request is to read data. */
bool MemoryRequest::isReadRequest() const {
  return getBits(startRWBit, startRWBit) == 1;
}

/* Return whether or not the request is for an entire instruction packet. */
bool MemoryRequest::isIPKRequest() const {
  return getBits(startIPKBit, startIPKBit) == 1;
}

/* Return whether or not this request is active. A request is deemed active
 * if there are still remaining data transfers to take place. */
bool MemoryRequest::isActive() const {
  return (getNumOps() != 0) || isIPKRequest();
}

/* Decrements the number of reads/writes remaining in this request. */
void MemoryRequest::decrementNumOps() {
  if(getNumOps() != 0) setNumOps(getNumOps() - 1);
}

/* Increments the address to read/write. To be used when an operation has
 * completed. */
void MemoryRequest::incrementAddress() {
  int strideLength = 1;
  setMemoryAddress(getMemoryAddress() + strideLength);
}

/* Sets whether the request is for an entire instruction packet. Should be
 * used to set "false" when the end of an instruction packet is reached. */
void MemoryRequest::setIPKRequest(bool val) {
  setBits(startIPKBit, startIPKBit, val?1:0);
}

/* Constructors and destructors */
MemoryRequest::MemoryRequest() : Word() {

}

MemoryRequest::MemoryRequest(short startAddr, short retAddr, short ops, bool read) :
    Word() {
  setMemoryAddress(startAddr);
  setReturnAddress(retAddr);
  setNumOps(ops);
  setRWBit(read);
}

MemoryRequest::MemoryRequest(const Word& other) : Word(other) {

}

MemoryRequest::~MemoryRequest() {

}

/* Private setter methods */
void MemoryRequest::setMemoryAddress(short val) {
  setBits(startStartAddr, startRetAddr - 1, val);
}

void MemoryRequest::setReturnAddress(short val) {
  setBits(startRetAddr, startNumOps - 1, val);
}

void MemoryRequest::setNumOps(short val) {
  setBits(startNumOps, startRWBit - 1, val);
}

void MemoryRequest::setRWBit(bool val) {
  setBits(startRWBit, startRWBit, val?1:0);
}
