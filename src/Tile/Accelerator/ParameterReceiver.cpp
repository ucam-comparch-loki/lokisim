/*
 * ParameterReceiver.cpp
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#include "ParameterReceiver.h"
#include "../../Utility/Assert.h"

ParameterReceiver::ParameterReceiver(sc_module_name name) :
    LokiComponent(name),
    iParameter("iParameter"),
    oReady("oReady") {

  parametersReceived = 0;

  SC_METHOD(receiveParameter);
  sensitive << iParameter;
  dont_initialize();

}

bool ParameterReceiver::hasAllParameters() const {
  return parametersReceived == sizeof(conv_parameters_t) / sizeof(uint32_t);
}

conv_parameters_t ParameterReceiver::getParameters() {
  parametersReceived = 0;
  return parameters;
}

const sc_event& ParameterReceiver::allParametersArrived() const {
  return allParametersArrivedEvent;
}

void ParameterReceiver::receiveParameter() {
  loki_assert(!hasAllParameters());
  loki_assert(iParameter.valid());
  loki_assert(oReady.read());

  uint32_t parameter = iParameter.read();
  iParameter.ack();

  // Add parameter to struct. (Using a naughty method so I don't need to care
  // about the contents of the struct.)
  uint32_t* array = reinterpret_cast<uint32_t*>(&parameters);
  array[parametersReceived] = parameter;
  parametersReceived++;

  // Trigger event if necessary.
  if (hasAllParameters())
    allParametersArrivedEvent.notify(sc_core::SC_ZERO_TIME);

  // TODO: Flow control for receiver - perhaps a separate method.
  // Or control the timing of the acknowledgement.
  // For now, always allow new input.
}
