/*
 * AcceleratorTile.h
 *
 * Extension of a ComputeTile which can also hold Accelerators.
 *
 *  Created on: 13 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATORTILE_H_
#define SRC_TILE_ACCELERATORTILE_H_

#include "ComputeTile.h"

class AcceleratorTile: public ComputeTile {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  AcceleratorTile(const sc_module_name& name, const ComponentID& id);
  virtual ~AcceleratorTile();


//============================================================================//
// Methods
//============================================================================//

public:

  // Magic network connection for cores and accelerators.
  void networkSendDataInternal(const NetworkData& flit);
  // There is also a method for sending credits, but accelerators don't use
  // credits.


//============================================================================//
// Components
//============================================================================//

private:

  vector<Accelerator*> accelerators;

};

#endif /* SRC_TILE_ACCELERATORTILE_H_ */
