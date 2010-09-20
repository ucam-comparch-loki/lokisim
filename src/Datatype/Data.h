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

  uint32_t getData() const;
  void moveBit(const int oldPos, const int newPos);

//==============================//
// Constructors and destructors
//==============================//

public:

  Data();
  Data(const Word& other);
  Data(const uint32_t data_);
  virtual ~Data();

};

#endif /* DATA_H_ */
