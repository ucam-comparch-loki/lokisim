/*
 * OrGate.h
 *
 *  Created on: 13 Sep 2011
 *      Author: db434
 */

#ifndef ORGATE_H_
#define ORGATE_H_

#include "Component.h"

class OrGate: public Component {

//==============================//
// Ports
//==============================//

public:

  LokiVector<sc_in<bool> > dataIn;
  sc_out<bool>             dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(OrGate);

  OrGate(const sc_module_name& name, int inputs) :
      Component(name) {
    dataIn.init(inputs);

    SC_METHOD(inputChanged);
    for(int i=0; i<dataIn.length(); i++) sensitive << dataIn[i];
    // do initialise
  }

//==============================//
// Methods
//==============================//

private:

  void inputChanged() {
    for(int i=0; i<dataIn.length(); i++) {
      if(dataIn[i].read()) {
        dataOut.write(true);
        return;
      }
    }

    dataOut.write(false);
  }

};


#endif /* ORGATE_H_ */
