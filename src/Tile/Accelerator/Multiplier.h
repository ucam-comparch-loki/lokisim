/*
 * Multiplier.h
 *
 * A single, parameterisable multiplier unit. Produces a new output whenever
 * any input changes.
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_MULTIPLIER_H_
#define SRC_TILE_ACCELERATOR_MULTIPLIER_H_

#include "../../LokiComponent.h"

// TODO Should there be separate types for the inputs and outputs? Perhaps even
// separate types for each input?
template <typename T>
class Multiplier: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  sc_in<T> in1, in2;
  sc_out<T> out;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Multiplier);

  // Create a Multiplier unit for data of type T. The unit produces an output
  // `latency` cycles after it receives an input, and is able to accept new
  // input every `initiationInterval` cycles.
  Multiplier(sc_module_name name, int latency, int initiationInterval) :
      LokiComponent(name),
      in1("in1"),
      in2("in2"),
      out("out") {

    // TODO implement latency and initiation interval.

    SC_METHOD(multiply);
    sensitive << in1 << in2;
    dont_initialize();

  }


//============================================================================//
// Methods
//============================================================================//

private:

  void multiply() {
    // For some datatypes, may also need to do some normalisation here. And/or
    // provide a higher-precision output.
    out.write(in1.read() * in2.read());
//    cout << this->name() << " multiplying " << in1.read() << " by " << in2.read() << endl;
  }

};

#endif /* SRC_TILE_ACCELERATOR_MULTIPLIER_H_ */
