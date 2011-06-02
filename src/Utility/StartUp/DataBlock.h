/*
 * DataBlock.h
 *
 * A simple grouping of a set of words to go into a memory, and the position
 * in memory for the first word to be placed.
 *
 *  Created on: 21 Oct 2010
 *      Author: db434
 */

#ifndef DATABLOCK_H_
#define DATABLOCK_H_

#include <vector>

#include "../../Datatype/ComponentID.h"

using std::vector;

class Word;

class DataBlock {

public:

  vector<Word>& data() const;
  ComponentID component() const;
  int position() const;

  DataBlock(vector<Word>* data, const ComponentID& component, int position=0);
  virtual ~DataBlock();

private:

  vector<Word>* data_;
  ComponentID component_;
  int position_;

};

#endif /* DATABLOCK_H_ */
