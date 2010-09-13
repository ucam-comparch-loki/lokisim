/*
 * Multiplexer3.h
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef MULTIPLEXER3_H_
#define MULTIPLEXER3_H_

#include "Multiplexer.h"

template<class T>
class Multiplexer3 : public Multiplexer<T> {

//==============================//
// Ports
//==============================//

public:

  sc_in<T> in1, in2, in3;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Multiplexer3);
  Multiplexer3(sc_module_name name) : Multiplexer<T>(name) {

  }

//==============================//
// Methods
//==============================//

protected:

  virtual void doOp() {
    switch(this->select.read()) {
      case(0) : this->result.write(in1.read()); break;
      case(1) : this->result.write(in2.read()); break;
      case(2) : this->result.write(in3.read()); break;
      default : throw new std::exception();
    }
  }

};

#endif /* MULTIPLEXER3_H_ */
