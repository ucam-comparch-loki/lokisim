/*
 * Data.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Data.h"

unsigned int Data::getData() {
  return data;
}

void Data::moveBit(int oldPos, int newPos) {
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

Data::Data(unsigned int data_) : Word(data_) {
  // Do nothing
}

Data::~Data() {

}
