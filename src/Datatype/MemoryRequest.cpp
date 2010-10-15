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
uint32_t MemoryRequest::address() const {
  return getBits(startAddress, startOperation - 1);
}

/* Return the number of reads/writes the client would like to carry out. */
uint8_t MemoryRequest::operation() const {
  return getBits(startOperation, end - 1);
}

/* Return whether or not the request is to read data. */
bool MemoryRequest::isReadRequest() const {
  short op = operation();
  return op == LOAD || op == LOAD_B || op == IPK_READ;
}

/* Return whether or not the request is for an entire instruction packet. */
bool MemoryRequest::isIPKRequest() const {
  return operation() == IPK_READ;
}

/* Increments the address to read/write. To be used when an operation has
 * completed. */
void MemoryRequest::incrementAddress() {
  int strideLength = 1;
  address(address() + strideLength);
}

/* Sets whether the request is for an entire instruction packet. Should be
 * used to set "false" when the end of an instruction packet is reached. */
void MemoryRequest::setIPKRequest(bool val) {
  operation(IPK_READ);
}

/* Constructors and destructors */
MemoryRequest::MemoryRequest() : Word() {

}

MemoryRequest::MemoryRequest(uint32_t addr, uint8_t opType) :
    Word() {
  address(addr);
  operation(opType);
}

MemoryRequest::MemoryRequest(const Word& other) : Word(other) {

}

MemoryRequest::~MemoryRequest() {

}

/* Private setter methods */
void MemoryRequest::address(uint32_t val) {
  setBits(startAddress, startOperation - 1, val);
}

void MemoryRequest::operation(uint8_t val) {
  setBits(startOperation, end - 1, val);
}
