/*
 * Network.cpp
 *
 *  Created on: 27 Mar 2019
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Network.h"
#include "../Utility/Assert.h"
#include "../Utility/Instrumentation/Network.h"

using sc_core::sc_module_name;
using std::set;

// Some parts of SystemC don't seem to interact very well with templated
// classes. The best solution I've found is to list out the types which can
// be used.
template class Network<Word>;

template<typename T>
Network<T>::Network(sc_module_name name, uint numInputs, uint numOutputs) :
    LokiComponent(name),
    clock("clock"),
    inputs("inputs", numInputs),
    outputs("outputs", numOutputs),
    requests(numOutputs),
    arbiters(numOutputs),
    copiesRemaining(numInputs, 0) {

  // Some more initialisation happens once all ports have been bound in
  // end_of_elaboration().
}

template<typename T>
void Network<T>::end_of_elaboration() {
  // Method to send data on each output port whenever possible.
  for (PortIndex output = 0; output < outputs.size(); output++)
    SPAWN_METHOD(requests[output].newRequestEvent(), Network<T>::sendData, output, false);

  // Method to monitor the head flit on each input port and determine which
  // output to use.
  for (PortIndex input = 0; input < inputs.size(); input++)
    SPAWN_METHOD(inputs[input]->canReadEvent(), Network<T>::newData, input, false);
}

template<typename T>
void Network<T>::reportStalls(ostream& os) {
  for (PortIndex i=0; i<inputs.size(); i++)
    if (inputs[i]->canRead())
      os << this->name() << " has data on input port " << i << ": " << inputs[i]->peek() << endl;
  for (PortIndex i=0; i<outputs.size(); i++)
    if (!outputs[i]->canWrite())
      os << this->name() << " unable to send to output port " << i << endl;
}

template<typename T>
set<PortIndex> Network<T>::getDestinations(const ChannelID address) const {
  // Default: defer to the single destination method.
  set<PortIndex> destination;
  destination.insert(getDestination(address));
  return destination;
}

template<typename T>
void Network<T>::sendData(PortIndex output) {
//  if (!clock.posedge())
//    next_trigger(clock.posedge_event());
//  else
  if (!outputs[output]->canWrite()) {
    next_trigger(outputs[output]->canWriteEvent());
  }
  else if (requests[output].empty()) {
    next_trigger(requests[output].newRequestEvent());
  }
  else {
    request_list_t& req = requests[output];

    PortIndex granted = arbiters[output].arbitrate(req);

    // Exit early if the selected input has no data available. This can happen
    // when in wormhole routing mode.
    if (!inputs[granted]->canRead()) {
      next_trigger(inputs[granted]->canReadEvent());
      return;
    }

    req.remove(granted);

    Flit<T> flit = readData(granted);
    Flit<T> previousFlit = outputs[output]->lastDataWritten();
    outputs[output]->write(flit);

    if (ENERGY_TRACE)
      Instrumentation::Network::crossbarOutput(previousFlit, flit);

    if (flit.getMetadata().endOfPacket) {
      arbiters[output].release();

      // Assume we can only do arbitration once per cycle - we can let multiple
      // flits through (if bandwidth available), but can't start another packet.
      next_trigger(clock.posedge_event());
    }
    else {
      arbiters[output].hold();
      // Default trigger: new request arrives
    }
  }
}

template<typename T>
Flit<T> Network<T>::readData(PortIndex input) {
  loki_assert(copiesRemaining[input] > 0);
  copiesRemaining[input]--;

  // Only consume the input if it is the final copy of a multicast message.
  if (copiesRemaining[input] == 0)
    return inputs[input]->read();
  else
    return inputs[input]->peek();
}

template<typename T>
void Network<T>::newData(PortIndex input) {
  loki_assert(inputs[input]->canRead());

  if (ENERGY_TRACE)
    Instrumentation::Network::crossbarInput(inputs[input]->lastDataRead(),
                                            inputs[input]->peek(), input);

  updateRequests(input);
}

template<typename T>
void Network<T>::updateRequests(PortIndex input) {
  loki_assert(inputs[input]->canRead());

  Flit<T> flit = inputs[input]->peek();
  set<PortIndex> targets = getDestinations(flit.channelID());
  copiesRemaining[input] = targets.size();

  for (auto it = targets.begin(); it != targets.end(); ++it) {
    PortIndex target = *it;
    loki_assert_with_message(target < outputs.size(), "Channel: %s, target port: %d, total ports: %d",
        flit.channelID().getString(Encoding::hardwareChannelID).c_str(), target, outputs.size());

    // TODO: Consider only adding the request if the output is available. This
    // could become useful when there are multiple buffers behind each port.
    // Look up how the Verilog handles this - might be overcomplicating things.
    requests[target].add(input);
  }
}
