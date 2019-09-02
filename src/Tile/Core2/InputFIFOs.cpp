/*
 * InputFIFOs.cpp
 *
 *  Created on: Aug 19, 2019
 *      Author: db434
 */

#include "InputFIFOs.h"

namespace Compute {

InputFIFOs::InputFIFOs(sc_module_name name, size_t numFIFOs,
                       const fifo_parameters_t params) :
    StorageBase<NetworkData, int32_t, NetworkData>(name),
    iData("iData", numFIFOs) {

  for (uint i=0; i<numFIFOs; i++) {
    // Can't use sc_gen_unique_name because it starts at 0, but we want names
    // to start at index 2 to account for the instruction inputs.

    std::stringstream name;
    name << "fifo_" << (i+2);

    NetworkFIFO<Word>* fifo =
        new NetworkFIFO<Word>(name.str().c_str(), params.size);
    fifos.push_back(fifo);

    iData[i](*fifo);
  }

}

const int32_t& InputFIFOs::debugRead(RegisterIndex reg) {
  return fifos[reg].peek().payload().toInt();
}

void InputFIFOs::debugWrite(RegisterIndex reg, NetworkData value) {
  fifos[reg].write(value);
}

void InputFIFOs::processRequests() {
  if (!this->clock.posedge()) {
    next_trigger(this->clock.posedge_event());
    return;
  }

  // Process writes first so reads see the latest data.
  for (uint i=0; i<this->writePort.size(); i++) {
    if (this->writePort[i].inProgress()) {
      fifos[this->writePort[i].reg()].write(this->writePort[i].result());
      this->writePort[i].notifyFinished();
    }
  }

  for (uint i=0; i<this->readPort.size(); i++)
    if (this->readPort[i].inProgress())
      this->readPort[i].setResult(fifos[this->readPort[i].reg()].read().payload().toInt());
}

} /* namespace Compute */
