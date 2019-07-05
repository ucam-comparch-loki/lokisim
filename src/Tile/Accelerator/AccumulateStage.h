/*
 * AccumulateStage.h
 *
 * Collapse an n-dimensional input down to an n-1-dimensional output by summing
 * all values in a given dimension.
 *
 * This corresponds to a 1D array of high-fan-in adders. (Or a tree of simpler
 * adders.)
 *
 *  Created on: 4 Jul 2019
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ACCUMULATESTAGE_H_
#define SRC_TILE_ACCELERATOR_ACCUMULATESTAGE_H_

#include "ComputeStage.h"

template <typename T>
class AccumulateStage: public ComputeStage<T> {

public:

  enum AccumulateOption {
    SUM_ROWS,     // Sum all values in each row
    SUM_COLUMNS   // Sum all values in each column
  };

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // Create adders for data of type T. The unit produces an output
  // `latency` cycles after it receives an input, and is able to accept new
  // input every `initiationInterval` cycles.
  AccumulateStage(sc_module_name name, AccumulateOption dimension, int latency,
                  int initiationInterval) :
      ComputeStage<T>(name, 1, latency, initiationInterval),
      dimension(dimension) {

  }

  virtual ~AccumulateStage() {}


//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void compute() {
    // For some datatypes, may also need to do some normalisation here.

    switch (dimension) {
      case SUM_ROWS: {

        for (uint row=0; row<this->in[0]->size().height; row++) {
          T sum = 0;

          for (uint col=0; col<this->in[0]->size().width; col++)
            sum += this->in[0]->read(row, col);

          this->out->write(row, 0, sum);
        }

        break;
      }

      case SUM_COLUMNS: {

        for (uint col=0; col<this->in[0]->size().width; col++) {
          T sum = 0;

          for (uint row=0; row<this->in[0]->size().height; row++)
            sum += this->in[0]->read(row, col);

          this->out->write(0, col, sum);
        }

        break;
      }
    } // end switch

  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The dimension to sum across.
  const AccumulateOption dimension;

};

#endif /* SRC_TILE_ACCELERATOR_ACCUMULATESTAGE_H_ */
