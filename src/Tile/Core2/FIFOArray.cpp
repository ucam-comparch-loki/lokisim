/*
 * FIFOArray.cpp
 *
 *  Created on: Aug 19, 2019
 *      Author: db434
 */

#include "FIFOArray.h"
#include <set>

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

void InputFIFOs::selectChannelWithData(DecodedInstruction inst, uint bitmask) {
  loki_assert(bitmask != 0);
  loki_assert(!selchInst);

  this->bitmask = bitmask;
  selchInst = inst;

  checkBitmask();
}

bool InputFIFOs::hasData(RegisterIndex fifo) const {
  return fifos[fifo].canRead();
}

RegisterIndex InputFIFOs::getSelectedChannel() const {
  RegisterIndex buffer = currentChannel.value();
  loki_assert(fifos[buffer].canRead());
  return buffer;
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
  for (uint port=0; port<this->writePort.size(); port++) {
    if (this->writePort[port].inProgress()) {
      fifos[this->writePort[port].reg()].write(this->writePort[port].result());

      // No callback needed - no instruction can write to the input FIFOs.

      this->writePort[port].clear();
      newData = true;
    }
  }

  if (newData)
    checkBitmask();  // For selch instruction.

  std::set<ChannelIndex> portsRead;
  for (uint port=0; port<this->readPort.size(); port++) {
    if (this->readPort[port].inProgress()) {
      loki_assert_with_message(portsRead.find(port) == portsRead.end(),
          "Read from input channel %d twice", port);

      RegisterIndex reg = this->readPort[port].reg();
      read_t result = fifos[reg].read().payload().toInt();
      this->readPort[port].instruction()->readRegistersCallback(port, result);
      this->readPort[port].clear();
      portsRead.insert(port);
    }
  }
}

void InputFIFOs::checkBitmask() {
  if (bitmask == 0)
    return;

  for (uint i=0; i<fifos.size(); i++) {
    RegisterIndex buffer = ++currentChannel;
    if (fifos[buffer].canRead() && (bitmask & (1 << buffer))) {
      selchInst->computeCallback(buffer);
      bitmask = 0;
      selchInst.reset();
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

void OutputFIFOs::write(DecodedInstruction inst, NetworkData value) {
  if (value.channelID().multicast)
    StorageBase<NetworkData>::write(inst, 1, value);
  else
    StorageBase<NetworkData>::write(inst, 0, value);
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
  for (uint port=0; port<this->writePort.size(); port++) {
    if (this->writePort[port].inProgress()) {
      fifos[this->writePort[port].reg()].write(this->writePort[port].result());
      this->writePort[port].instruction()->sendNetworkDataCallback();
      this->writePort[port].clear();
    }
  }

  for (uint port=0; port<this->readPort.size(); port++) {
    if (this->readPort[port].inProgress()) {
      RegisterIndex reg = this->readPort[port].reg();
      read_t result = fifos[reg].read();

      // No callback - no instruction can read from the output FIFOs.

      this->readPort[port].clear();
    }
  }
}

} /* namespace Compute */
