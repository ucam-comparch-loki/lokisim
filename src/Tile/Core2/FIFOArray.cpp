/*
 * FIFOArray.cpp
 *
 *  Created on: Aug 19, 2019
 *      Author: db434
 */

#include "FIFOArray.h"

namespace Compute {

InputFIFOs::InputFIFOs(sc_module_name name, size_t numFIFOs,
                       const fifo_parameters_t params) :
    StorageBase<NetworkData, int32_t, NetworkData>(name, 2, 0), // Writing handled by FIFOs
    iData("iData", numFIFOs),
    currentChannel(numFIFOs) {

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

  local = 0;

}

bool InputFIFOs::hasData(RegisterIndex fifo) const {
  return fifos[fifo].canRead();
}

void InputFIFOs::selectChannelWithData(uint bitmask) {
  // TODO
  // Don't return anything, just set state to monitor these channels.
  // Trigger an event if data is already here.
}

RegisterIndex InputFIFOs::getSelectedChannel() const {
  // TODO
}

const int32_t& InputFIFOs::debugRead(RegisterIndex fifo) {
  local = fifos[fifo].peek().payload().toInt();
  return local;
}

void InputFIFOs::debugWrite(RegisterIndex fifo, NetworkData value) {
  fifos[fifo].write(value);
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

  // TODO: warn if reading from the same FIFO multiple times.
  for (uint i=0; i<this->readPort.size(); i++)
    if (this->readPort[i].inProgress())
      this->readPort[i].setResult(fifos[this->readPort[i].reg()].read().payload().toInt());
}


OutputFIFOs::OutputFIFOs(sc_module_name name, const fifo_parameters_t params) :
    StorageBase<NetworkData>(name, 0, 1), // Reading is handled by FIFOs
    oMemory("oMemory"),
    oMulticast("oMulticast") {

  NetworkFIFO<Word>* memoryBuf =
      new NetworkFIFO<Word>("memoryBuf", params.size);
  fifos.push_back(memoryBuf);
  oMemory(*memoryBuf);

  NetworkFIFO<Word>* multicastBuf =
      new NetworkFIFO<Word>("multicastBuf", params.size);
  fifos.push_back(multicastBuf);
  oMulticast(*multicastBuf);

}

void OutputFIFOs::write(NetworkData value) {
  if (value.channelID().multicast)
    StorageBase<NetworkData>::write(1, value);
  else
    StorageBase<NetworkData>::write(0, value);
}

const NetworkData& OutputFIFOs::debugRead(RegisterIndex fifo) {
  local = fifos[fifo].peek();
  return local;
}

void OutputFIFOs::debugWrite(RegisterIndex fifo, NetworkData value) {
  fifos[fifo].write(value);
}

void OutputFIFOs::processRequests() {
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
      this->readPort[i].setResult(fifos[this->readPort[i].reg()].read());
}

} /* namespace Compute */
