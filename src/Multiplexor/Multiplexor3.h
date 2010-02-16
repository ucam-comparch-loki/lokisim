/*
 * Multiplexor3.h
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef MULTIPLEXOR3_H_
#define MULTIPLEXOR3_H_

#include "Multiplexor.h"

template<class T>
class Multiplexor3 : public Multiplexor<T> {

public:
/* Ports */
  sc_in<T> in1, in2, in3;

/* Constructors and destructors */
  SC_HAS_PROCESS(Multiplexor3);
  Multiplexor3(sc_core::sc_module_name name) : Multiplexor<T>(name) {

  }

protected:
/* Methods */
  virtual void doOp() {
    switch(Multiplexor<T>::select.read()) {
      case(0) : Multiplexor<T>::result.write(in1.read()); break;
      case(1) : Multiplexor<T>::result.write(in2.read()); break;
      case(2) : Multiplexor<T>::result.write(in3.read()); break;
      default : throw new std::exception();
    }
  }

};

#endif /* MULTIPLEXOR3_H_ */
