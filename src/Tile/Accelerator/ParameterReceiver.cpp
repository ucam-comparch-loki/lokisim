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
    clock("clock"),
    iParameter("iParameter"),
    paramBuffer("buf", 1, 100) {

  parametersReceived = 0;

  paramBuffer.clock(clock);
  iParameter(paramBuffer);

  SC_METHOD(receiveParameter);
  sensitive << paramBuffer.canReadEvent();
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
  loki_assert(paramBuffer.canRead());

  uint32_t parameter = paramBuffer.read().payload().toUInt();

  // Add parameter to struct. (Using a naughty method so I don't need to care
  // about the contents of the struct.)
  uint32_t* array = reinterpret_cast<uint32_t*>(&parameters);
  array[parametersReceived] = parameter;
  parametersReceived++;

  LOKI_LOG(2) << this->name() << ": received parameter " << parameter << " ("
      << parametersReceived << "/" << (sizeof(conv_parameters_t)/sizeof(uint32_t))
      << ")" << endl;

  // Trigger event if necessary.
  if (hasAllParameters()) {
    allParametersArrivedEvent.notify(sc_core::SC_ZERO_TIME);
    LOKI_LOG(2) << this->name() << ": has all parameters" << endl;
  }

  // TODO: Flow control for receiver - perhaps a separate method.
  // Or control the timing of the acknowledgement.
  // For now, always allow new input.
}
