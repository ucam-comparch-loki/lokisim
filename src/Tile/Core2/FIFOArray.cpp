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

  bitmask = 0;
  local = 0;

}

bool InputFIFOs::hasData(RegisterIndex fifo) const {
  return fifos[fifo].canRead();
}

void InputFIFOs::selectChannelWithData(uint bitmask) {
  loki_assert(bitmask != 0);
  this->bitmask = bitmask;

  // TODO
  // Don't return anything, just set state to monitor these channels.
  // Trigger selectedDataArrived if data is already here. Reset the bitmask when
  // that event is notified.
}

RegisterIndex InputFIFOs::getSelectedChannel() const {
  RegisterIndex buffer = currentChannel.value();
  loki_assert(fifos[buffer].canRead());
  return buffer;
}

const sc_event& InputFIFOs::anyDataArrivedEvent() const {
  return anyDataArrived;
}

const sc_event& InputFIFOs::selectedDataArrivedEvent() const {
  return selectedDataArrived;
}

const int32_t& InputFIFOs::debugRead(RegisterIndex fifo) {
  // Need to return a reference to match existing interfaces, so copy to a local
  // variable first.
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

  bool newData = false;

  // Process writes first so reads see the latest data.
  for (uint i=0; i<this->writePort.size(); i++) {
    if (this->writePort[i].inProgress()) {
      fifos[this->writePort[i].reg()].write(this->writePort[i].result());
      this->writePort[i].notifyFinished();
      newData = true;
    }
  }

  if (newData) {
    anyDataArrived.notify(sc_core::SC_ZERO_TIME);
    checkBitmask();  // For selch instruction.
  }

  // TODO: warn if reading from the same FIFO multiple times.
  for (uint i=0; i<this->readPort.size(); i++)
    if (this->readPort[i].inProgress())
      this->readPort[i].setResult(fifos[this->readPort[i].reg()].read().payload().toInt());
}

void InputFIFOs::checkBitmask() {
  if (bitmask == 0)
    return;

  for (uint i=0; i<fifos.size(); i++) {
    RegisterIndex buffer = ++currentChannel;
    if (fifos[buffer].canRead() && (bitmask & (1 << buffer))) {
      selectedDataArrived.notify(sc_core::SC_ZERO_TIME);
      bitmask = 0;
      break;
    }
  }
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

const sc_event& OutputFIFOs::anyDataSentEvent() const {
  return anyDataSent;
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
      anyDataSent.notify(sc_core::SC_ZERO_TIME);
    }
  }

  for (uint i=0; i<this->readPort.size(); i++)
    if (this->readPort[i].inProgress())
      this->readPort[i].setResult(fifos[this->readPort[i].reg()].read());
}

} /* namespace Compute */
