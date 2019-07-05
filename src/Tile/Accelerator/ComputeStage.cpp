/*
 * ComputeStage.cpp
 *
 *  Created on: 5 Jul 2019
 *      Author: db434
 */

#include "ComputeStage.h"
#include "ComputeUnit.h"

// Need to list all options for templated components.
template class ComputeStage<int>;
template class ComputeStage<float>;

// The stage produces an output `latency` cycles after it receives an input,
// and is able to accept new input every `initiationInterval` cycles.
template<typename T>
ComputeStage<T>::ComputeStage(sc_module_name name, uint numInputs, uint latency,
                              uint initiationInterval) :
    LokiComponent(name),
    BlockingInterface(),
    in("in", numInputs),
    out("out") {

  // TODO implement latency and initiation interval.
  // Latency affects when we call out->finishedWriting
  // II affects when we call in->finishedReading

}

template<typename T>
void ComputeStage<T>::end_of_elaboration() {
  SC_METHOD(mainLoop);
  for (uint i=0; i<in.size(); i++)
    sensitive << in[i]->canReadEvent();
  dont_initialize();
}

// Ensure all inputs and outputs are ready, and then trigger the compute
// method.
template<typename T>
void ComputeStage<T>::mainLoop() {
  // Ensure all inputs are available.
  for (uint i=0; i<in.size(); i++)
    if (!in[i]->canRead())
      return;   // Default trigger: new data arrives.

  // Ensure all inputs correspond to the same tick.
  tick_t tick = in[0]->getTick();
  for (uint i=1; i<in.size(); i++)
    if (in[i]->getTick() != tick)
      return;   // Default trigger: new data arrives.

  // Ensure output is available.
  if (!out->canWrite())
    next_trigger(out->canWriteEvent());

  // Main work.
  compute();

  // Flow control.
  out->finishedWriting(tick);
  for (uint i=0; i<in.size(); i++)
    in[i]->finishedReading();
}

template<typename T>
void ComputeStage<T>::reportStalls(ostream& os) {
  // Ignore inputs? It's quite common not to have data to process.
  if (!out->canWrite())
    os << this->name() << " unable to write to output" << endl;
}

template<typename T>
ComputeUnit<T>& ComputeStage<T>::parent() const {
  return *(static_cast<ComputeUnit<T>*>(this->get_parent_object()));
}

template<typename T>
const ComponentID& ComputeStage<T>::id() const {
  return parent().id();
}


