/*
 * DataBlock.cpp
 *
 *  Created on: 21 Oct 2010
 *      Author: db434
 */

#include "DataBlock.h"
#include "../../Datatype/Word.h"
#include "../../Datatype/Identifier.h"

vector<Word>& DataBlock::payload() const {
  return *data_;
}

ComponentID DataBlock::component() const {
  return component_;
}

int DataBlock::position() const {
  return position_;
}

bool DataBlock::readOnly() const {
  return readOnly_;
}

DataBlock::DataBlock(vector<Word>* data, const ComponentID& component, int position, bool readOnly) :
  data_(data),
  component_(component),
  position_(position),
  readOnly_(readOnly) {

}

DataBlock::~DataBlock() {
  // For some reason, data_ can't be deleted in this class. We must therefore
  // use (from somewhere else):
  //   delete &(block.data());
}
