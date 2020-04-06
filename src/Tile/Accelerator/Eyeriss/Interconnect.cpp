/*
 * Interconnect.cpp
 *
 *  Created on: Feb 13, 2020
 *      Author: db434
 */

#include "Interconnect.h"

namespace Eyeriss {

template<typename T>
MulticastBus<T>::MulticastBus(sc_module_name name) :
    LokiComponent(name) {
  numStalled = 0;
}

template<typename T>
const sc_event& MulticastBus<T>::default_event() const {
  return newDataEvent;
}

template<typename T>
const T MulticastBus<T>::read() const {
  return data;
}

template<typename T>
void MulticastBus<T>::stall() {
  if (numStalled == 0)
    stalledEvent.notify(sc_core::SC_ZERO_TIME);

  numStalled++;
}

template<typename T>
void MulticastBus<T>::unstall() {
  loki_assert(numStalled > 0);
  numStalled--;

  if (numStalled == 0)
    unstalledEvent.notify(sc_core::SC_ZERO_TIME);
}


template<typename T>
const sc_event& MulticastBus<T>::stallEvent() const {
  return stalledEvent;
}

template<typename T>
const sc_event& MulticastBus<T>::unstallEvent() const {
  return unstalledEvent;
}

template<typename T>
void MulticastBus<T>::write(T data) {
  loki_assert(ready());
  this->data = data;
}

template<typename T>
bool MulticastBus<T>::ready() const {
  return numStalled == 0;
}

} /* namespace Eyeriss */
