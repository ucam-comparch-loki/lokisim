/*
 * ConnectionStatus.cpp
 *
 *  Created on: 31 Mar 2010
 *      Author: db434
 */

#include "ConnectionStatus.h"
#include "../Datatype/MemoryRequest.h"
#include "../Utility/Parameters.h"

/* Tells whether there is a connection set up, but not currently carrying
 * out an operation. */
bool ConnectionStatus::idle() const {
  return operation_ == NONE;
}

bool ConnectionStatus::isRead() const {
  return operation_ == LOAD || operation_ == LOADHW || operation_ == LOADBYTE;
}

bool ConnectionStatus::isWrite() const {
  return operation_ == STORE || operation_ == STOREHW || operation_ == STOREBYTE;
}

bool ConnectionStatus::streaming() const {
  // TODO: streams should probably know how long they will last, so they can
  // be stopped when they are finished.
  return repeatOperation_;
}

bool ConnectionStatus::readingIPK() const {
  // This will not always be valid: we may eventually want to stream data too.
  return streaming() && isRead();
}

bool ConnectionStatus::byteAccess() const {
  return operation_ == LOADBYTE || operation_ == STOREBYTE;
}

bool ConnectionStatus::halfWordAccess() const {
  return operation_ == LOADHW || operation_ == STOREHW;
}

void ConnectionStatus::incrementAddress() {
  if(byteAccess())    address_ += stride_;
  else if(halfWordAccess()) address_ += stride_ * (BYTES_PER_WORD/2);
  else if(readingIPK()) address_ += stride_ * BYTES_PER_INSTRUCTION;
  else                  address_ += stride_ * BYTES_PER_WORD;
}

/* End the current operation, but keep the connection up so new operations
 * still have their results sent back to the same place. */
void ConnectionStatus::clear() {
  address_ = UNUSED;
  operation_ = NONE;
  repeatOperation_ = false;
  stride_ = 1;
}

/* Completely remove the connection, allowing a new component to connect to
 * this port. */
void ConnectionStatus::teardown() {
  clear();
}

MemoryAddr ConnectionStatus::address() const {
  return address_;
}

void ConnectionStatus::readAddress(MemoryAddr addr, int operation) {
  address_ = addr;
  if(operation == MemoryRequest::LOAD_W || operation == MemoryRequest::IPK_READ)
    operation_ = LOAD;
  else if(operation == MemoryRequest::LOAD_HW) operation_ = LOADHW;
  else if(operation == MemoryRequest::LOAD_B) operation_ = LOADBYTE;
  else std::cerr << "Unknown memory operation in ConnectionStatus::readAddress: "
                 << operation << std::endl;
}

void ConnectionStatus::writeAddress(MemoryAddr addr, int operation) {
  address_ = addr;
  if(operation == MemoryRequest::STORE_W) operation_ = STORE;
  else if(operation == MemoryRequest::STORE_HW) operation_ = STOREHW;
  else if(operation == MemoryRequest::STORE_B) operation_ = STOREBYTE;
  else if(operation == MemoryRequest::STADDR) {
    operation_ = STORE;
    repeatOperation_ = true;
  }
  else if(operation == MemoryRequest::STBADDR) {
    operation_ = STOREBYTE;
    repeatOperation_ = true;
  }
  else std::cerr << "Unknown memory operation in ConnectionStatus::writeAddress: "
                 << operation << std::endl;
}

void ConnectionStatus::startStreaming() {
  repeatOperation_ = true;
}

void ConnectionStatus::stopStreaming() {
  repeatOperation_ = false;
}

ConnectionStatus::ConnectionStatus() {
  address_       = UNUSED;
  operation_     = NONE;
  stride_        = 1;
}
