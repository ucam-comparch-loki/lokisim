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

  // The data to be stored.
  vector<Word>& payload() const;

  // The component (core or memory) to store the data in. A null ComponentID
  // signifies the background memory.
  ComponentID component() const;

  // The address at which the payload should be stored.
  int position() const;

  // Marks whether this data may be changed by the program.
  bool readOnly() const;

  DataBlock(vector<Word>* data, const ComponentID& component, int position, bool readOnly);
  virtual ~DataBlock();

private:

  vector<Word>* data_;
  ComponentID component_;
  int position_;
  bool readOnly_;

};

#endif /* DATABLOCK_H_ */
