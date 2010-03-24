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

  friend void sc_trace(sc_core::sc_trace_file*& tf, const Data& w, const std::string& txt) {
    sc_core::sc_trace(tf, (int)(w.data >> 32), txt);
  }

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
