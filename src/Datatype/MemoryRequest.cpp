/*
 * MemoryRequest.cpp
 *
 * Current layout:
 *    29 bit read/write address
 *    3 bit operation (read/write/fetch)
 *    Lots of bits spare (could include a stride length?)
 *
 *    |          spare           | operation |       address       |
 *     63                         31          28                   0
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#include "MemoryRequest.h"

const short startAddress   = 0;
const short startOperation = startAddress + 29;
const short end            = startOperation + 3;

/* Return the memory address to read from or write to. */
unsigned int MemoryRequest::getAddress() const {
  return getBits(startAddress, startOperation - 1);
}

/* Return the number of reads/writes the client would like to carry out. */
unsigned short MemoryRequest::getOperation() const {
  return getBits(startOperation, end - 1);
}

/* Return whether or not the request is to read data. */
bool MemoryRequest::isReadRequest() const {
  return getOperation() == LOAD || getOperation() == IPK_READ;
}

/* Return whether or not the request is for an entire instruction packet. */
bool MemoryRequest::isIPKRequest() const {
  return getOperation() == IPK_READ;
}

/* Increments the address to read/write. To be used when an operation has
 * completed. */
void MemoryRequest::incrementAddress() {
  int strideLength = 1;
  setAddress(getAddress() + strideLength);
}

/* Sets whether the request is for an entire instruction packet. Should be
 * used to set "false" when the end of an instruction packet is reached. */
void MemoryRequest::setIPKRequest(bool val) {
  setOperation(IPK_READ);
}

/* Constructors and destructors */
MemoryRequest::MemoryRequest() : Word() {

}

MemoryRequest::MemoryRequest(short address, short operation) :
    Word() {
  setAddress(address);
  setOperation(operation);
}

MemoryRequest::MemoryRequest(const Word& other) : Word(other) {

}

MemoryRequest::~MemoryRequest() {

}

/* Private setter methods */
void MemoryRequest::setAddress(int val) {
  setBits(startAddress, startOperation - 1, val);
}

void MemoryRequest::setOperation(short val) {
  setBits(startOperation, end - 1, val);
}
