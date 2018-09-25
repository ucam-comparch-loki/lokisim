/*
 * ScalarVector.h
 *
 * Component which multiplies all elements of a vector by the same scalar
 * value.
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_SCALARVECTOR_H_
#define SRC_TILE_ACCELERATOR_SCALARVECTOR_H_

#include "../../LokiComponent.h"

template <typename T>
class ScalarVector: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  sc_in<T>              inScalar;
  LokiVector<sc_in<T>>  inVector;
  LokiVector<sc_out<T>> out;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ScalarVector(int length, int latency, int initiationInterval) :
      LokiComponent("scalar_vector"),
      inVector(length, "in_vector"),
      out(length, "out") {

    for (uint i=0; i<length; i++) {
      Multiplier<T>* multiplier = new Multiplier<T>(latency, initiationInterval);
      multiplier->in1(inScalar);
      multiplier->in2(inVector[i]);
      multiplier->out(out[i]);

      multipliers.push_back(multiplier);
    }

  }

//============================================================================//
// Components
//============================================================================//

private:

  LokiVector<Multiplier<T>> multipliers;

};

#endif /* SRC_TILE_ACCELERATOR_SCALARVECTOR_H_ */
