/*
 * DMA.h
 *
 * Units responsible for autonomously fetching or storing blocks of data.
 *
 *  Created on: 17 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_DMA_H_
#define SRC_TILE_ACCELERATOR_DMA_H_

#include "../../LokiComponent.h"
#include "AcceleratorTypes.h"
#include "../../Utility/Assert.h"

typedef Flit<Word> MemoryRequest;

template <typename T>
class DMABase: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Tell whether the command queue is able to accept a new command.
  sc_out<bool> oCmdQueueReady;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMABase);

  // Also include cache details?
  DMABase(sc_module_name name, size_t queueLength=4) :
      LokiComponent(name),
      commandQueue(queueLength) {

    SC_METHOD(updateReady);
    sensitive << commandQueue.queueChangedEvent();
    // do initialise
  }


//============================================================================//
// Methods
//============================================================================//

public:

  void enqueueCommand(const dma_command_t command) {
    commandQueue.enqueue(command);
  }

  bool canAcceptCommand() const {
    return !commandQueue.full();
  }

  void replaceMemoryMapping(uint32_t mapEncoded) {
    memoryMapping = ChannelMapEntry::memoryView(mapEncoded);
  }

protected:

  const dma_command_t dequeueCommand() {
    return commandQueue.dequeue();
  }

  MemoryRequest buildRequest(MemoryAddr address, MemoryOpcode op) {
    // TODO - use channel map entry to determine which memory bank to access.
    // Might be good to extract this out to a component shared with Cores.
  }

private:

  void updateReady() {
    bool ready = canAcceptCommand();
    if (ready != oCmdQueueReady.read())
      oCmdQueueReady.write(ready);
  }


//============================================================================//
// Local state
//============================================================================//

private:

  // Commands from control unit telling which data to load/store.
  CommandQueue commandQueue;

  // Memory configuration.
  ChannelMapEntry::MemoryChannel memoryMapping;

};

template <typename T>
class DMAInput: public DMABase<T> {

public:

  LokiVector<sc_out<T>> oDataToPEs;
  sc_out<bool>          oDataReady;

};

template <typename T>
class DMAOutput: public DMABase<T> {

public:

  LokiVector<sc_in<T>> iDataFromPEs;
  sc_in<bool>          iDataReady;

};


#endif /* SRC_TILE_ACCELERATOR_DMA_H_ */
