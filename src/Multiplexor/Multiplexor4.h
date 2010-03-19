/*
 * Multiplexor4.h
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef MULTIPLEXOR4_H_
#define MULTIPLEXOR4_H_

#include "Multiplexor.h"

template<class T>
class Multiplexor4 : public Multiplexor<T> {

//==============================//
// Ports
//==============================//

public:

  sc_in<T> in1, in2, in3, in4;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Multiplexor4);
  Multiplexor4(sc_module_name name) : Multiplexor<T>(name) {

  }

//==============================//
// Methods
//==============================//

protected:

  virtual void doOp() {
    switch(Multiplexor<T>::select.read()) {
      case(0) : Multiplexor<T>::result.write(in1.read()); break;
      case(1) : Multiplexor<T>::result.write(in2.read()); break;
      case(2) : Multiplexor<T>::result.write(in3.read()); break;
      case(3) : Multiplexor<T>::result.write(in4.read()); break;
      default : throw new std::exception();
    }
  }

};

#endif /* MULTIPLEXOR4_H_ */
