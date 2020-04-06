/*
 * Eyeriss::PE.cpp
 *
 *  Created on: Feb 11, 2020
 *      Author: db434
 */

#include "PE.h"

namespace Eyeriss {

PE::PE(sc_module_name name) :
    LokiComponent(name),
    clock("clock"),
    control("control"),
    activationInput("act_in"),
    weightInput("weight_in"),
    psumInput("psum_in"),
    psumOutput("psum_out") {

  numFilters = numChannels = outputShift = -1;

  SC_METHOD(reconfigure);
  sensitive << control;
  dont_initialize();

  SC_METHOD(newActivationArrived);
  sensitive << activationInput;
  dont_initialize();

  SC_METHOD(newWeightArrived);
  sensitive << weightInput;
  dont_initialize();

  SC_METHOD(newPSumArrived);
  sensitive << psumInput;
  dont_initialize();

  SC_METHOD(sendPSums);
  sensitive << psumOutputFIFO.writeEvent();
  dont_initialize();

  // TODO: extend FIFO so it works with the new interface?
  SC_METHOD(activationFlowControl);
  // sensitive << activationInputFIFO.something

  SC_METHOD(weightFlowControl);
  // sensitive << weightInputFIFO.something

  SC_METHOD(psumFlowControl);
  // sensitive << psumInputFIFO.something

  // TODO: computation
}

void PE::reconfigure() {
  // TODO: assert not busy with computation

  numFilters = control->getNumFilters();
  numChannels = control->getNumChannels();
  outputShift = control->getOutputShift();
}

void PE::newActivationArrived() {
  activationInputFIFO.write(activationInput->read());
}

void PE::newWeightArrived() {
  weightInputFIFO.write(weightInput->read());
}

void PE::newPSumArrived() {
  psumInputFIFO.write(psumInput->read());
}

void PE::sendPSums() {
  loki_assert(psumOutput->ready());
  psumOutput->write(psumOutputFIFO.read());
}

} // end Eyeriss namespace
