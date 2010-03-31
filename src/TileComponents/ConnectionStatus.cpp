/*
 * ConnectionStatus.cpp
 *
 *  Created on: 31 Mar 2010
 *      Author: db434
 */

#include "ConnectionStatus.h"
#include "stdio.h"

/* Tells whether there is a connection set up at all. */
bool ConnectionStatus::isActive() const {
  return remoteChannel != UNUSED;
}

/* Tells whether there is a connection set up, but not currently carrying
 * out an operation. */
bool ConnectionStatus::isIdle() const {
  return isActive() && (operation == NONE);
}

bool ConnectionStatus::isRead() const {
  return operation == LOAD;
}

bool ConnectionStatus::isWrite() const {
  return operation == STORE;
}

bool ConnectionStatus::isStreaming() const {
  return repeatOperation;
}

bool ConnectionStatus::readingIPK() const {
  return repeatOperation && (operation == LOAD);
}

void ConnectionStatus::incrementAddress() {
  address += STRIDE;
}

void ConnectionStatus::clear() {
  address = UNUSED;
  operation = NONE;
  repeatOperation = false;
}

void ConnectionStatus::teardown() {
  remoteChannel   = UNUSED;
  clear();
}

int ConnectionStatus::getChannel() const {
  return remoteChannel;
}

int ConnectionStatus::getAddress() const {
  return address;
}

void ConnectionStatus::setChannel(int channel) {
  remoteChannel = channel;
}

void ConnectionStatus::setReadAddress(int addr) {
  address = addr;
  operation = LOAD;
}

void ConnectionStatus::setWriteAddress(int addr) {
  address = addr;
  operation = STORE;
}

void ConnectionStatus::startStreaming() {
  repeatOperation = true;
}

void ConnectionStatus::stopStreaming() {
  repeatOperation = false;
}

ConnectionStatus::ConnectionStatus() {
  remoteChannel = UNUSED;
  address       = UNUSED;
  operation     = NONE;
}

ConnectionStatus::~ConnectionStatus() {

}
