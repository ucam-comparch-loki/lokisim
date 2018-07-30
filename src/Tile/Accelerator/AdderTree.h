/*
 * AdderTree.h
 *
 * Component which sums together all inputs. A new output is computed whenever
 * any input changes.
 *
 *  Created on: 2 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ADDERTREE_H_
#define SRC_TILE_ACCELERATOR_ADDERTREE_H_

#include "../../LokiComponent.h"

template <typename T>
class AdderTree: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  LokiVector<sc_in<T>> in;
  sc_out<T> out;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(AdderTree);

  AdderTree(int length, int latency, int initiationInterval) :
      LokiComponent("adder_tree"),
      in(length, "in") {

    // TODO implement latency and initiation interval

    SC_METHOD(add);
    for (uint i=0; i<in.size(); i++)
      sensitive << in[i];
    dont_initialize();

  }


//============================================================================//
// Methods
//============================================================================//

private:

  void add() {
    T sum = 0;

    for (uint i=0; i<in.size(); i++)
      sum += in[i].read();

    out.write(sum);
  }
};

#endif /* SRC_TILE_ACCELERATOR_ADDERTREE_H_ */
