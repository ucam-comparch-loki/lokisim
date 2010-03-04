/*
 * MemoryRequest.cpp
 *
 * Current layout:
 *    16 bit read/write address
 *    16 bit return address
 *    8 bit number of operations to do (e.g. request 8 sequential reads at once)
 *    1 read/write bit (1 = read, 0 = write)
 *    15 bits spare (could include a stride length?)
 *
 *    |    spare     | r/w bit | num ops | return address | start address |
 *     63             40        39        31               15             0
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#include "MemoryRequest.h"

const short startStartAddr = 0;
const short startRetAddr = 16;
const short startNumOps = 32;
const short startRWBit = 40;

/* Public getter methods */
unsigned short MemoryRequest::getStartAddress() const {
  return getBits(startStartAddr, startRetAddr - 1);
}

unsigned short MemoryRequest::getReturnAddress() const {
  return getBits(startRetAddr, startNumOps - 1);
}

unsigned short MemoryRequest::getNumOps() const {
  return getBits(startNumOps, startRWBit - 1);
}

bool MemoryRequest::isReadRequest() const {
  return getBits(startRWBit, startRWBit) == 1;
}

bool MemoryRequest::isActive() const {
  return getNumOps() != 0;
}

void MemoryRequest::decrementNumOps() {
  setNumOps(getNumOps() - 1);
}

void MemoryRequest::incrementAddress() {
  int strideLength = 1;
  setStartAddress(getStartAddress() + strideLength);
}

/* Constructors and destructors */
MemoryRequest::MemoryRequest() : Word() {

}

MemoryRequest::MemoryRequest(short startAddr, short retAddr, short ops, bool read) :
    Word() {
  setStartAddress(startAddr);
  setReturnAddress(retAddr);
  setNumOps(ops);
  setRWBit(read);
}

MemoryRequest::MemoryRequest(const Word& other) : Word(other) {

}

MemoryRequest::~MemoryRequest() {

}

/* Private setter methods */
void MemoryRequest::setStartAddress(short val) {
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
