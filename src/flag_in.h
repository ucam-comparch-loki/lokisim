/*
 * flag_in.h
 *
 * Delete?
 *
 *  Created on: 1 Mar 2010
 *      Author: db434
 */

#ifndef FLAG_IN_H_
#define FLAG_IN_H_

#include "systemc"

using sc_core::sc_in;

template<class T>
class flag_in : public sc_in<T> {

public:
/* Methods */
  bool newData() {
//    bool returnVal = newDataFlag;
//    newDataFlag = false;
    return this->event();//returnVal;
  }

  virtual const char* kind() const {
    return "flag_in";
  }

/* Constructors and destructors */
  flag_in() : sc_in<T>() {
    newDataFlag = false;
  }

private:
/* Local state */
  bool newDataFlag;

};


#endif /* FLAG_IN_H_ */
