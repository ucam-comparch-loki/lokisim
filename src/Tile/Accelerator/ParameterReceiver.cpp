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
  totalParameters = 0;

  paramBuffer.clock(clock);
  iParameter(paramBuffer);

  SC_METHOD(receiveParameter);
  sensitive << paramBuffer.canReadEvent();
  dont_initialize();

}

bool ParameterReceiver::hasAllParameters() const {
  // We don't find out how many parameters are needed until we receive the
  // second parameter (number of loops).
  return (parametersReceived > 2) && (parametersReceived == totalParameters);
}

lat_parameters_t ParameterReceiver::getParameters() {
  loki_assert(hasAllParameters());

  parametersReceived = 0;
  totalParameters = 0;

  lat_parameters_t toReturn = parameters;
  parameters = lat_parameters_t();

  return toReturn;
}

const sc_event& ParameterReceiver::allParametersArrived() const {
  return allParametersArrivedEvent;
}

void ParameterReceiver::receiveParameter() {
  loki_assert(!hasAllParameters());
  loki_assert(paramBuffer.canRead());

  uint32_t parameter = paramBuffer.read().payload().toUInt();

  int loopParams = 0;
  int iterationParams = 0;

  // Need to receive 2 parameters before we know how many loops there are.
  if (parametersReceived >= 2) {
    loopParams = parameters.loop_count * 3;
    iterationParams = parameters.loop_count * 1;
  }

  // Add parameter to struct. (The struct has dynamically-sized fields, so I
  // can't just cast it to an int array.)
  // Using sizeof here is dangerous because Loki's sizes don't always match the
  // host's.

  if (parametersReceived == 0)
    parameters.notification_address = parameter;
  else if (parametersReceived == 1) {
    parameters.loop_count = parameter;

    loopParams = parameters.loop_count * 3;
    iterationParams = parameters.loop_count * 1;
    totalParameters = 2 + loopParams + iterationParams + 3 * 2;
  }
  else if (parametersReceived < 2 + loopParams) {
    // Position within one loop_iteration_t
    int offset = (parametersReceived - 2) % 3;

    if (offset == 0)
      parameters.loops.push_back(loop_iteration_t());

    loop_iteration_t& loop = parameters.loops.back();
    uint32_t* array = reinterpret_cast<uint32_t*>(&loop);
    array[offset] = parameter;
  }
  else if (parametersReceived < 2 + loopParams + iterationParams) {
    parameters.iteration_counts.push_back(parameter);
  }
  else {
    // Hack: the rest of the struct is fixed size, so treat it as an int array.
    uint32_t* array = reinterpret_cast<uint32_t*>(&parameters.in1);
    array[parametersReceived - 2 - loopParams - iterationParams] = parameter;
  }

  parametersReceived++;

  LOKI_LOG(2) << this->name() << ": received parameter " << parameter << " ("
      << parametersReceived << "/" << ((totalParameters == 0) ? "?" : std::to_string(totalParameters))
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
