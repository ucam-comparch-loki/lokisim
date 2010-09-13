/*
 * Multiplexer2.h
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef MULTIPLEXER2_H_
#define MULTIPLEXER2_H_

#include "Multiplexer.h"

template<class T>
class Multiplexer2 : public Multiplexer<T> {

//==============================//
// Ports
//==============================//

public:

  sc_in<T> in1, in2;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Multiplexer2);
  Multiplexer2(sc_module_name name) : Multiplexer<T>(name) {

  }

//==============================//
// Methods
//==============================//

protected:

  virtual void doOp() {
    switch(this->select.read()) {
      case(0) : this->result.write(in1.read()); break;
      case(1) : this->result.write(in2.read()); break;
      default : throw new std::exception();
    }
  }

};

#endif /* MULTIPLEXER2_H_ */
