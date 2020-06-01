/*
 * MultiplierStage.h
 *
 * A 2D grid of multipliers.
 *
 * Each multiplier accepts two inputs and produces one output. Inputs may be
 * shared between multipliers, but all outputs are unique.
 *
 *  Created on: 4 Jul 2019
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_MULTIPLIERSTAGE_H_
#define SRC_TILE_ACCELERATOR_MULTIPLIERSTAGE_H_

#include "../../Utility/Instrumentation/Operations.h"
#include "ComputeStage.h"

template <typename T>
class MultiplierStage: public ComputeStage<T> {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // Create multipliers for data of type T. The unit produces an output
  // `latency` cycles after it receives an input, and is able to accept new
  // input every `initiationInterval` cycles.
  MultiplierStage(sc_module_name name, size2d_t size, int latency,
                  int initiationInterval) :
      ComputeStage<T>(name, 2, latency, initiationInterval),
      size(size) {

  }

  virtual ~MultiplierStage() {}


//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void compute() {
    // For some datatypes, may also need to do some normalisation here. And/or
    // provide a higher-precision output.

    for (uint row=0; row<size.height; row++) {
      for (uint col=0; col<size.width; col++) {
        T result = this->in[0]->read(row, col) * this->in[1]->read(row, col);
        this->out->write(row, col, result);

        if (this->in[0]->read(row, col) != 0 && this->in[1]->read(row, col) != 0)
          LOKI_LOG(4) << this->name() << " PE (" << row << "," << col << "): " <<
              this->in[0]->read(row, col) << " x " <<
              this->in[1]->read(row, col) << " = " << result << endl;
      }
    }

    // TODO: log more information, e.g. number of active PEs.
    Instrumentation::Operations::acceleratorTick(this->id(), size.total());

  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The size of the multiplier array.
  const size2d_t size;

};



#endif /* SRC_TILE_ACCELERATOR_MULTIPLIERSTAGE_H_ */
