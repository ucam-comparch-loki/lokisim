/*
 * CommandQueue.h
 *
 *  Created on: 17 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_COMMANDQUEUE_H_
#define SRC_TILE_ACCELERATOR_COMMANDQUEUE_H_

#include <queue>
#include <systemc>
#include "AcceleratorTypes.h"

using sc_core::sc_event;

class CommandQueue {

public:

  CommandQueue(size_t maxLength);

  void enqueue(const dma_command_t command);
  const dma_command_t dequeue();

  bool full() const;
  bool empty() const;

  const sc_event& queueChangedEvent() const;

private:

  std::queue<dma_command_t> queue;
  const size_t maxLength;
  sc_event queueChanged;

};

#endif /* SRC_TILE_ACCELERATOR_COMMANDQUEUE_H_ */
