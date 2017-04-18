/*
 * MagicMemoryConnection.h
 *
 * Component responsible for managing communication with a zero-latency memory.
 * The component should go unused unless MAGIC_MEMORY is set to 1.
 *
 *  Created on: 30 Jan 2017
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_MAGICMEMORYCONNECTION_H_
#define SRC_TILE_CORE_MAGICMEMORYCONNECTION_H_

#include "../../Datatype/DecodedInst.h"
#include "../../LokiComponent.h"
#include "../../Memory/MemoryTypes.h"

class Core;

class MagicMemoryConnection: public LokiComponent {
public:
  MagicMemoryConnection(sc_module_name name, ComponentID id);
  virtual ~MagicMemoryConnection();

  // Receive notification that the given instruction wants to send a request to
  // memory. It should already have computed all of its results.
  void operate(const DecodedInst& instruction);

private:
  Core* parent() const;

  // Many operations are self-contained, but some (especially sendconfigs)
  // require us to maintain state between instructions.
  MemoryOpcode currentOpcode;
  MemoryAddr   currentAddress;
  unsigned int payloadsRemaining;

};

#endif /* SRC_TILE_CORE_MAGICMEMORYCONNECTION_H_ */
