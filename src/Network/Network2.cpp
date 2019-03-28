/*
 * Network2.cpp
 *
 *  Created on: 27 Mar 2019
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Network2.h"
#include "../Utility/Assert.h"
#include "../Utility/Instrumentation/Network.h"

using sc_core::sc_module_name;

// Some parts of SystemC don't seem to interact very well with templated
// classes. The best solution I've found is to list out the types which can
// be used.
template class Network2<Word>;

template<typename T>
Network2<T>::Network2(sc_module_name name, uint numInputs, uint numOutputs) :
    LokiComponent(name),
    inputs("inputs", numInputs),
    outputs("outputs", numOutputs),
    requests(numOutputs),
    arbiters(numOutputs) {

  // Some more initialisation happens once all ports have been bound in
  // end_of_elaboration().
}

template<typename T>
void Network2<T>::end_of_elaboration() {
  // Method to send data on each output port whenever possible.
  for (PortIndex output = 0; output < outputs.size(); output++)
    SPAWN_METHOD(requests[output].newRequestEvent(), Network2<T>::sendData, output, false);

  // Method to monitor each input port and determine which output to use.
  for (PortIndex input = 0; input < inputs.size(); input++)
    SPAWN_METHOD(inputs[input]->dataAvailableEvent(), Network2<T>::newData, input, false);
}

template<typename T>
void Network2<T>::reportStalls(ostream& os) {
  for (PortIndex i=0; i<inputs.size(); i++)
    if (inputs[i]->dataAvailable())
      os << this->name() << " has data on input port " << i << ": " << inputs[i]->peek() << endl;
  for (PortIndex i=0; i<outputs.size(); i++)
    if (!outputs[i]->canWrite())
      os << this->name() << " unable to send to output port " << i << endl;
}

template<typename T>
void Network2<T>::sendData(PortIndex output) {
  if (!clock.posedge())
    next_trigger(clock.posedge_event());
  else if (!outputs[output]->canWrite())
    next_trigger(outputs[output]->canWriteEvent());
  else {
    request_list_t& req = requests[output];

    PortIndex granted = arbiters[output].arbitrate(req);

    // Exit early if the selected input has no data available. This can happen
    // when in wormhole routing mode.
    if (!inputs[granted]->dataAvailable()) {
      next_trigger(inputs[granted]->dataAvailableEvent());
      return;
    }

    // TODO: repeat this step multiple times if bandwidth > 1.
    // Stop as soon as buffer empties, packet ends, or bandwidth reached.
    // Assert that until packet ends, all flits have same destination.
    Flit<T> flit = inputs[granted]->read();
    Flit<T> previousFlit = outputs[output]->lastDataWritten();
    outputs[output]->write(flit);

    if (ENERGY_TRACE)
        Instrumentation::Network::crossbarOutput(previousFlit, flit);

    if (flit.getMetadata().endOfPacket)
      arbiters[output].release();
    else
      arbiters[output].hold();

    req.remove(granted);
    updateRequests(granted);

    if (req.empty())
      next_trigger(req.newRequestEvent());
    else
      next_trigger(clock.posedge_event());
  }
}

template<typename T>
void Network2<T>::newData(PortIndex input) {
  loki_assert(inputs[input]->dataAvailable());

  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarInput(inputs[input]->lastDataRead(),
                                            inputs[input]->peek(), input);

  updateRequests(input);
}

template<typename T>
void Network2<T>::updateRequests(PortIndex input) {
  if (!inputs[input]->dataAvailable())
    return; // Do nothing.

  Flit<T> flit = inputs[input]->peek();
  PortIndex output = getDestination(flit.channelID());
  loki_assert(output < outputs.size());

  // TODO: Consider only adding the request if the output is available. This
  // could become useful when there are multiple buffers behind each port.
  // Look up how the Verilog handles this - might be overcomplicating things.
  requests[output].add(input);
}
