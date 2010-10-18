/*
 * ConnectionStatus.cpp
 *
 *  Created on: 31 Mar 2010
 *      Author: db434
 */

#include "ConnectionStatus.h"

/* Tells whether there is a connection set up at all. */
bool ConnectionStatus::active() const {
  return remoteChannel_ != UNUSED;
}

/* Tells whether there is a connection set up, but not currently carrying
 * out an operation. */
bool ConnectionStatus::idle() const {
  return active() && (operation_ == NONE);
}

bool ConnectionStatus::isRead() const {
  return operation_ == LOAD;
}

bool ConnectionStatus::isWrite() const {
  return operation_ == STORE;
}

bool ConnectionStatus::streaming() const {
  return repeatOperation_;
}

bool ConnectionStatus::readingIPK() const {
  return streaming() && isRead();
}

bool ConnectionStatus::isByteAccess() const {
  return (operation_ == LOADBYTE || operation_ == STOREBYTE);
}

void ConnectionStatus::incrementAddress() {
  if(isByteAccess()) address_ += STRIDE;
  else               address_ += STRIDE * BYTES_PER_WORD;
}

void ConnectionStatus::clear() {
  address_ = UNUSED;
  operation_ = NONE;
  repeatOperation_ = false;
}

void ConnectionStatus::teardown() {
  remoteChannel_   = UNUSED;
  clear();
}

int ConnectionStatus::channel() const {
  return remoteChannel_;
}

int ConnectionStatus::address() const {
  return address_;
}

void ConnectionStatus::channel(int channel) {
  remoteChannel_ = channel;
}

void ConnectionStatus::readAddress(int addr) {
  address_ = addr;
  operation_ = LOAD;
}

void ConnectionStatus::writeAddress(int addr) {
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
