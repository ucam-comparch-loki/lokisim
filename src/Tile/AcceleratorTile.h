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

class Accelerator;

class AcceleratorTile: public ComputeTile {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  AcceleratorTile(const sc_module_name& name, const TileID& id,
                  const tile_parameters_t& params);


//============================================================================//
// Methods
//============================================================================//

public:

  virtual uint numComponents() const;
  virtual uint numAccelerators() const;
  virtual bool isCore(ComponentID id) const;
  virtual bool isMemory(ComponentID id) const;
  virtual bool isAccelerator(ComponentID id) const;
  virtual uint componentIndex(ComponentID id) const;
  virtual uint coreIndex(ComponentID id) const;
  virtual uint memoryIndex(ComponentID id) const;
  virtual uint acceleratorIndex(ComponentID id) const;

  // Magic network connection for cores and accelerators.
  void networkSendDataInternal(const NetworkData& flit);
  // There is also a method for sending credits, but accelerators don't use
  // credits.


//============================================================================//
// Components
//============================================================================//

private:

  LokiVector<Accelerator> accelerators;

};

#endif /* SRC_TILE_ACCELERATORTILE_H_ */
