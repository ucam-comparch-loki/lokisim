/*
 * DotProduct.h
 *
 * Component which computes the element-wise product of two input vectors.
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_DOTPRODUCT_H_
#define SRC_TILE_ACCELERATOR_DOTPRODUCT_H_

#include "../../LokiComponent.h"

template <typename T>
class DotProduct: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Vectors of inputs and outputs.
  LokiVector<sc_in<T>>  in1, in2;
  LokiVector<sc_out<T>> out;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  DotProduct(int length, int latency, int initiationInterval) :
      LokiComponent("dot_product"),
      in1(length, "in1"),
      in2(length, "in2"),
      out(length, "out") {

    for (uint i=0; i<length; i++) {
      Multiplier<T>* multiplier = new Multiplier<T>(latency, initiationInterval);
      multiplier->in1(in1[i]);
      multiplier->in2(in2[i]);
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

#endif /* SRC_TILE_ACCELERATOR_DOTPRODUCT_H_ */
