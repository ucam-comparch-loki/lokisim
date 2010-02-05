/*
 * Data.h
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef DATA_H_
#define DATA_H_

#include "Word.h"

class Data: public Word {

public:
  unsigned int getData();
  void moveBit(int oldPos, int newPos);

  Data();
  Data(const Word& other);
  Data(unsigned int data_);
  virtual ~Data();
};

#endif /* DATA_H_ */
