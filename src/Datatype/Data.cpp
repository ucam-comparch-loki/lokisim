/*
 * Data.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Data.h"

uint32_t Data::getData() const {
  return (uint32_t)data;
}

void Data::moveBit(const int oldPos, const int newPos) {
  unsigned int currentBit = getBits(oldPos, oldPos);
  clearBits(oldPos, oldPos);
  setBits(newPos, newPos, currentBit);
}

Data::Data() : Word() {
  // Do nothing
}

Data::Data(const Word& other) : Word(other) {
  // Do nothing
}

Data::Data(const uint32_t data_) : Word(data_) {
  // Do nothing
}

Data::~Data() {

}
