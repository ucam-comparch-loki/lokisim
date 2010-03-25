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

//==============================//
// Methods
//==============================//

public:

  unsigned int getData() const;
  void moveBit(int oldPos, int newPos);

//==============================//
// Constructors and destructors
//==============================//

public:

  Data();
  Data(const Word& other);
  Data(unsigned int data_);
  virtual ~Data();

};

#endif /* DATA_H_ */
