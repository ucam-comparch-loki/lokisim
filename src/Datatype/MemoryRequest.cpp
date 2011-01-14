/*
 * MemoryRequest.cpp
 *
 * Current layout:
 *    28 bit read/write address
 *    4 bit operation (read/write/fetch/setup/etc.)
 *    Lots of bits spare (could include a stride length?)
 *
 *    |          spare           | operation |       address       |
 *     63                         31          27                   0
 *
 * Since setup requests have to be hand coded at the moment, the setup
 * operation has value 0. This means that the setup request only involves
 * sending the channel ID of the port where we want the data from memory to go.
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#include "MemoryRequest.h"

const short startAddress   = 0;
const short startOperation = startAddress + 28;
const short end            = startOperation + 4;

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

bool MemoryRequest::isWriteRequest() const {
  short op = operation();
  return op == STORE || op == STORE_B || op == STADDR || op == STBADDR;
}

/* Return whether or not the request is for an entire instruction packet. */
bool MemoryRequest::isIPKRequest() const {
  return operation() == IPK_READ;
}

bool MemoryRequest::isSetup() const {
  return operation() == SETUP;
}

bool MemoryRequest::streaming() const {
  short op = operation();
  return op == IPK_READ || op == STADDR || op == STBADDR;
}

bool MemoryRequest::byteAccess() const {
  short op = operation();
  return op == LOAD_B || op == STORE_B;
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

MemoryRequest::MemoryRequest(MemoryAddr addr, uint8_t opType) :
    Word() {
  address(addr);
  operation(opType);
}

MemoryRequest::MemoryRequest(const Word& other) : Word(other) {

}

MemoryRequest::~MemoryRequest() {

}

/* Private setter methods */
void MemoryRequest::address(MemoryAddr val) {
  setBits(startAddress, startOperation - 1, val);
}

void MemoryRequest::operation(uint8_t val) {
  setBits(startOperation, end - 1, val);
}
