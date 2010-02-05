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
unsigned int Address::getAddress() const {
  return getBits(boundary, 31);
}

unsigned int Address::getChannelID() const {
  return getBits(1, boundary-1);
}

bool Address::getReadBit() const {
  // 1 = read, 0 = write
  return getBits(0,0);
}

/* Used to extract some bits and put them in the indirection registers. */
unsigned int Address::getLowestBits(int limit) const {
  return getBits(0, limit);
}

/* Constructors and destructors */
Address::Address() : Word() {
  // Do nothing
}

Address::Address(const Word& other) : Word(other) {

}

Address::Address(int addr, int channelID) : Word() {
  data = (addr << boundary) + channelID;
}

Address::~Address() {

}
