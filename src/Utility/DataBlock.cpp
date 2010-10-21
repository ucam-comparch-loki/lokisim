/*
 * DataBlock.cpp
 *
 *  Created on: 21 Oct 2010
 *      Author: db434
 */

#include "DataBlock.h"
#include "../Datatype/Word.h"

vector<Word>& DataBlock::data() const {
  return *data_;
}

int DataBlock::position() const {
  return position_;
}

DataBlock::DataBlock(vector<Word>* data, int position) {
  data_ = data;
  position_ = position;
}

DataBlock::~DataBlock() {
//  delete data_;
}