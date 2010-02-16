/*
 * TestClass.h
 *
 *  Created on: 16 Feb 2010
 *      Author: db434
 */

#ifndef TESTCLASS_H_
#define TESTCLASS_H_

#include "../Component.h"

class TestClass : public Component {

public:
/* Ports */
  sc_in<bool>* inputs;
  sc_out<bool>* outputs;

  void doOp() {
    for(int i=0; i<width; i++) {
      outputs[i].write(inputs[i].read());
    }
  }

/* Constructors and destructors */
  SC_HAS_PROCESS(TestClass);
  TestClass(sc_core::sc_module_name name, int ID, int width) :
      Component(name, ID) {

    this->width = width;

    inputs = new sc_in<bool>[width];
    outputs = new sc_out<bool>[width];
    buffers = new sc_buffer<bool>[width];

    SC_METHOD(doOp);
    for(int i=0; i<width; i++) {
      sensitive << inputs[i];
    }

  }

private:
/* Signals */
  sc_buffer<bool>* buffers;
  int width;

};

#endif /* TESTCLASS_H_ */
