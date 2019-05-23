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
    inputBuffer("inputBuffer", 1) {

  parametersReceived = 0;

  iParameter(inputBuffer);

  SC_METHOD(receiveParameter);
  sensitive << inputBuffer.canReadEvent();
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
  loki_assert(inputBuffer.canRead());

  uint32_t parameter = inputBuffer.read().payload().toUInt();

  // Add parameter to struct. (Using a naughty method so I don't need to care
  // about the contents of the struct.)
  uint32_t* array = reinterpret_cast<uint32_t*>(&parameters);
  array[parametersReceived] = parameter;
  parametersReceived++;

  LOKI_LOG << this->name() << ": received parameter " << parameter << " ("
      << parametersReceived << "/" << (sizeof(conv_parameters_t)/sizeof(uint32_t))
      << ")" << endl;

  // Trigger event if necessary.
  if (hasAllParameters()) {
    allParametersArrivedEvent.notify(sc_core::SC_ZERO_TIME);
    LOKI_LOG << this->name() << ": has all parameters" << endl;
  }
}
