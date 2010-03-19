/*
 * Multiplexor2.h
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef MULTIPLEXOR2_H_
#define MULTIPLEXOR2_H_

#include "Multiplexor.h"

template<class T>
class Multiplexor2 : public Multiplexor<T> {

//==============================//
// Ports
//==============================//

public:

  sc_in<T> in1, in2;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Multiplexor2);
  Multiplexor2(sc_module_name name) : Multiplexor<T>(name) {

  }

//==============================//
// Methods
//==============================//

protected:

  virtual void doOp() {
    switch(Multiplexor<T>::select.read()) {
      case(0) : Multiplexor<T>::result.write(in1.read()); break;
      case(1) : Multiplexor<T>::result.write(in2.read()); break;
      default : throw new std::exception();
    }
  }

};

#endif /* MULTIPLEXOR2_H_ */
