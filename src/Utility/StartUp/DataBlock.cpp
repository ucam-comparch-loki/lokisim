/*
 * DataBlock.cpp
 *
 *  Created on: 21 Oct 2010
 *      Author: db434
 */

#include "DataBlock.h"
#include "../../Datatype/Word.h"
#include "../../Datatype/ComponentID.h"

vector<Word>& DataBlock::data() const {
  return *data_;
}

ComponentID DataBlock::component() const {
  return component_;
}

int DataBlock::position() const {
  return position_;
}

DataBlock::DataBlock(vector<Word>* data, const ComponentID& component, int position) {
  data_ = data;
  component_ = component;
  position_ = position;
}

DataBlock::~DataBlock() {
  // For some reason, since data_ wasn't allocated in this class, it can't
  // be deleted in this class. We must therefore use (from somewhere else):
  //   delete &(block.data());
}
