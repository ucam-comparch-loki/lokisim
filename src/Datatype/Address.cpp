/*
 * Address.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "Address.h"

/*
 * Current layout: 32 bit value containing 16 bit channelID and 16 bit address.
 * Swap them around?
 * There is also a bit saying whether a value is being read or written to.
 *                    |    Address    |   ChannelID  | Read/write bit |
 *                    31              15             1                0
 */

/* Boundary between address and channel ID */
const short boundary = 16;

/* Accessing information */
uint16_t Address::address() const {
  return getBits(boundary, 31);
}

uint16_t Address::channelID() const {
  return getBits(1, boundary-1);
}

bool Address::readBit() const {
  // 1 = read, 0 = write
  return getBits(0,0);
}

Address& Address::addOffset(const int offset) {
  address(address() + offset);
  return *this;
}

/* Constructors and destructors */
Address::Address() : Word() {
  // Do nothing
}

Address::Address(const Word& other) : Word(other) {

}

Address::Address(const uint16_t addr, const uint16_t channel) : Word() {
  address(addr);
  channelID(channel);

  if(channel < 0) std::cerr << "Warning: creating address with channel ID of "
                            << channel << std::endl;
}

Address::~Address() {

}

void Address::address(const uint16_t addr) {
  setBits(boundary, 31, addr);
}

void Address::channelID(const uint16_t channelID) {
  setBits(1, boundary-1, channelID);
}

void Address::setRWBit(const bool read) {
  if(read) setBits(0, 0, 1);
  else setBits(0, 0, 0);
}
