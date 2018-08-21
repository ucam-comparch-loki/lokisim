/*
 * CommandQueue.cpp
 *
 *  Created on: 17 Jul 2018
 *      Author: db434
 */

#include "CommandQueue.h"

CommandQueue::CommandQueue(size_t maxLength) : maxLength(maxLength) {

  // Nothing

}

void CommandQueue::enqueue(const dma_command_t command) {
  assert(!full());

  queue.push(command);
  queueChanged.notify(sc_core::SC_ZERO_TIME);
}

const dma_command_t CommandQueue::dequeue() {
  assert(!empty());

  const dma_command_t result = queue.front();
  queue.pop();
  queueChanged.notify(sc_core::SC_ZERO_TIME);
  return result;
}

bool CommandQueue::full() const {
  return queue.size() == maxLength;
}

bool CommandQueue::empty() const {
  return queue.size() == 0;
}

const sc_core::sc_event& CommandQueue::queueChangedEvent() const {
  return queueChanged;
}
