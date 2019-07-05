/*
 * AdderStage.h
 *
 * A 2D grid of adders.
 *
 * Each adder accepts two inputs and produces one output. Inputs may be
 * shared between adders, but all outputs are unique.
 *
 *  Created on: 4 Jul 2019
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ADDERSTAGE_H_
#define SRC_TILE_ACCELERATOR_ADDERSTAGE_H_

#include "ComputeStage.h"


template <typename T>
class AdderStage: public ComputeStage<T> {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // Create adders for data of type T. The unit produces an output
  // `latency` cycles after it receives an input, and is able to accept new
  // input every `initiationInterval` cycles.
  AdderStage(sc_module_name name, size2d_t size, int latency,
             int initiationInterval) :
      ComputeStage<T>(name, 2, latency, initiationInterval),
      size(size) {

  }

  virtual ~AdderStage() {}


//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void compute() {

    for (uint row=0; row<size.height; row++) {
      for (uint col=0; col<size.width; col++) {
        T result = this->in[0]->read(row, col) + this->in[1]->read(row, col);
        this->out->write(row, col, result);
      }
    }

  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The size of the adder array.
  const size2d_t size;

};



#endif /* SRC_TILE_ACCELERATOR_ADDERSTAGE_H_ */
