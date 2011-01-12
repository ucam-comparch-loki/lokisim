/*
 * ConnectionStatus.cpp
 *
 *  Created on: 31 Mar 2010
 *      Author: db434
 */

#include "ConnectionStatus.h"
#include "../Utility/Parameters.h"

/* Tells whether there is a connection set up at all. */
bool ConnectionStatus::active() const {
  return (int)remoteChannel_ != UNUSED;
}

/* Tells whether there is a connection set up, but not currently carrying
 * out an operation. */
bool ConnectionStatus::idle() const {
  return active() && (operation_ == NONE);
}

bool ConnectionStatus::isRead() const {
  return operation_ == LOAD || operation_ == LOADBYTE;
}

bool ConnectionStatus::isWrite() const {
  return operation_ == STORE || operation_ == STOREBYTE;
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

bool ConnectionStatus::isByteAccess() const {
  return (operation_ == LOADBYTE || operation_ == STOREBYTE);
}

void ConnectionStatus::incrementAddress() {
  if(isByteAccess()) address_ += STRIDE;
  else               address_ += STRIDE * BYTES_PER_WORD;
}

/* End the current operation, but keep the connection up so new operations
 * still have their results sent back to the same place. */
void ConnectionStatus::clear() {
  address_ = UNUSED;
  operation_ = NONE;
  repeatOperation_ = false;
}

/* Completely remove the connection, allowing a new component to connect to
 * this port. */
void ConnectionStatus::teardown() {
  remoteChannel_   = UNUSED;
  clear();
}

ChannelID ConnectionStatus::channel() const {
  return remoteChannel_;
}

MemoryAddr ConnectionStatus::address() const {
  return address_;
}

void ConnectionStatus::channel(ChannelID channel) {
  remoteChannel_ = channel;
}

void ConnectionStatus::readAddress(MemoryAddr addr) {
  address_ = addr;
  operation_ = LOAD;
}

void ConnectionStatus::writeAddress(MemoryAddr addr) {
  address_ = addr;
  operation_ = STORE;
}

void ConnectionStatus::startStreaming() {
  repeatOperation_ = true;
}

void ConnectionStatus::stopStreaming() {
  repeatOperation_ = false;
}

ConnectionStatus::ConnectionStatus() {
  remoteChannel_ = UNUSED;
  address_       = UNUSED;
  operation_     = NONE;
}

ConnectionStatus::~ConnectionStatus() {

}
