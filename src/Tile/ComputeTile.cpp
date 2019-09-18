/*
 * ComputeTile.cpp
 *
 *  Created on: 4 Oct 2016
 *      Author: db434
 */

#include "ComputeTile.h"
#include "Core/Core.h"
#include "Memory/MemoryBank.h"
#include "../Chip.h"
#include "../Utility/Assert.h"
#include "../Utility/StartUp/DataBlock.h"

uint ComputeTile::numComponents() const {return numCores() + numMemories();}
uint ComputeTile::numCores() const      {return cores.size();}
uint ComputeTile::numMemories() const   {return memories.size();}

bool ComputeTile::isCore(ComponentID id) const {
  return id.position < numCores();
}

bool ComputeTile::isMemory(ComponentID id) const {
  return id.position >= numCores();
}

uint ComputeTile::componentIndex(ComponentID id) const {
  uint index = id.position;
  loki_assert(index < numComponents());
  return index;
}

uint ComputeTile::coreIndex(ComponentID id) const {
  uint index = id.position;
  loki_assert(index < numCores());
  return index;
}

uint ComputeTile::memoryIndex(ComponentID id) const {
  uint index = id.position - numCores();
  loki_assert(index < numMemories());
  return index;
}

uint ComputeTile::globalCoreIndex(ComponentID id) const {
  return chip().globalCoreIndex(id);
}

uint ComputeTile::globalMemoryIndex(ComponentID id) const {
  return chip().globalMemoryIndex(id);
}

void ComputeTile::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  if (component.tile != id)
    chip().storeInstructions(instructions, component);
  else {
    loki_assert(isCore(component));
    cores[coreIndex(component)].storeData(instructions);
  }
}

void ComputeTile::storeData(const DataBlock& data) {
  if (data.component().tile != id) {
    chip().storeData(data);
  }
  else if (isCore(data.component())) {
    cores[coreIndex(data.component())].storeData(data.payload());
  }
  else if (isMemory(data.component())) {
    LOKI_ERROR << "storing directly to memory banks is disabled since it cannot be known "
        "which bank the core will access." << endl;
//    assert(data.component().globalMemoryNumber() < memories.size());
//    memories[data.component().globalMemoryNumber()]->storeData(data.payload(),data.position());
  }
}

void ComputeTile::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
  if (component.tile != id || MAGIC_MEMORY)
    chip().print(component, start, end);
  else if (isMemory(component))
    memories[memoryIndex(component)].print(start, end);
}

Word ComputeTile::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.tile == id && isMemory(component) && !MAGIC_MEMORY)
    return memories[memoryIndex(component)].readWordDebug(addr);
  else
    return chip().readWordInternal(component, addr);
}

Word ComputeTile::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.tile == id && isMemory(component) && !MAGIC_MEMORY)
    return memories[memoryIndex(component)].readByteDebug(addr);
  else
    return chip().readByteInternal(component, addr);
}

void ComputeTile::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.tile == id && isMemory(component) && !MAGIC_MEMORY)
    memories[memoryIndex(component)].writeWordDebug(addr, data);
  else
    chip().writeWordInternal(component, addr, data);
}

void ComputeTile::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.tile == id && isMemory(component) && !MAGIC_MEMORY)
    memories[memoryIndex(component)].writeByteDebug(addr, data);
  else
    chip().writeByteInternal(component, addr, data);
}

int  ComputeTile::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  if (component.tile != id)
    return chip().readRegisterInternal(component, reg);
  else {
    loki_assert(isCore(component));
    return cores[coreIndex(component)].readRegDebug(reg);
  }
}

bool ComputeTile::readPredicateInternal(const ComponentID& component) const {
  if (component.tile != id)
    return chip().readPredicateInternal(component);
  else {
    loki_assert(isCore(component));
    return cores[coreIndex(component)].readPredReg();
  }
}

void ComputeTile::networkSendDataInternal(const NetworkData& flit) {
  if (flit.channelID().component.tile != id)
    chip().networkSendDataInternal(flit);
  else {
    loki_assert(isCore(flit.channelID().component));
    cores[coreIndex(flit.channelID().component)].deliverDataInternal(flit);
  }
}

void ComputeTile::networkSendCreditInternal(const NetworkCredit& flit) {
  if (flit.channelID().component.tile != id)
    chip().networkSendCreditInternal(flit);
  else {
    loki_assert(isCore(flit.channelID().component));
    cores[coreIndex(flit.channelID().component)].deliverCreditInternal(flit);
  }
}

MemoryAddr ComputeTile::getAddressTranslation(MemoryAddr address) const {
  return mhl.getAddressTranslation(address);
}

bool ComputeTile::backedByMainMemory(MemoryAddr address) const {
  return mhl.backedByMainMemory(address);
}

void ComputeTile::makeComponents(const tile_parameters_t& params) {

  // Initialise the cores of this tile
  for (uint core = 0; core < params.numCores; core++) {
    ComponentID coreID(id, core);

    Core* c = new Core(sc_gen_unique_name("core"), coreID, params.core,
                       params.mcastNetOutputs(), params.mcastNetInputs(),
                       params.numMemories);

    cores.push_back(c);
  }

  // Initialise the memories of this tile
  for (uint mem = 0; mem < params.numMemories; mem++) {
    ComponentID memoryID(id, params.numCores + mem);

    MemoryBank* m = new MemoryBank(sc_gen_unique_name("memory"), memoryID,
                                   params.numMemories, params.memory, params.numCores);
    m->setBackgroundMemory(&(chip().mainMemory));

    memories.push_back(m);
  }

}

void ComputeTile::wireUp(const tile_parameters_t& params) {

  invertedClock(clock);

  uint instructionInputs = Core::numInstructionChannels;
  uint totalInputs = params.core.numInputChannels;

  creditBuffer.clock(clock);

  for (uint i=0; i<cores.size(); i++) {
    Core& core = cores[i];

    core.clock(clock);

    coreToMemory.inputs[i](core.oMemory);
    coreToCore.inputs[i](core.oMulticast);
    creditReturn.outputs[i](core.iCredit);

    for (uint j=0; j<totalInputs; j++) {
      icu.iFlowControl[i][j](core.iData[j]);
      coreToCore.outputs[i * totalInputs + j](core.iData[j]);

      if (j < instructionInputs)
        instructionReturn.outputs[i * instructionInputs + j](core.iData[j]);

      // The return crossbar actually connects to instruction inputs as well.
      // It also handles data coming from the router.
      dataReturn.outputs[i * totalInputs + j](core.iData[j]);
    }
  }

  for (uint i=0; i<memories.size(); i++) {
    MemoryBank& memory = memories[i];

    memory.iClock(invertedClock);

    coreToMemory.outputs[i](memory.iData);
    dataReturn.inputs[i](memory.oData);
    instructionReturn.inputs[i](memory.oInstruction);
    bankToMHLRequests.inputs[i](memory.oRequest);
    mhlToBankResponses.outputs[i](memory.iResponse);
    bankToL2LResponses.inputs[i](memory.oResponse);

    memory.iRequest(l2lToBankRequests);
  }

  mhl.clock(clock);

  bankToMHLRequests.outputs[0](mhl.iRequestFromBanks);
  mhlToBankResponses.inputs[0](mhl.oResponseToBanks);

  l2l.oRequestToBanks(l2lToBankRequests);
  bankToL2LResponses.outputs[0](l2l.iResponseFromBanks);

  dataReturn.inputs[dataReturn.inputs.size()-1](icu.oData);

  coreToCore.clock(clock);
  coreToMemory.clock(clock);
  dataReturn.clock(clock);
  instructionReturn.clock(clock);
  creditReturn.clock(clock);
  bankToMHLRequests.clock(clock);
  mhlToBankResponses.clock(clock);
  bankToL2LResponses.clock(clock);

  // The global data network is connected to the final input of the return
  // crossbar, and to the final output of the forward crossbar.
  coreToMemory.outputs[coreToMemory.outputs.size()-1](oData);
  iData(icu.iData);

  icu.clock(clock);
  icu.oCredit(oCredit);

  iCredit(creditBuffer);
  creditReturn.inputs[0](creditBuffer);

  mhl.oRequestToNetwork(oRequest);
  iResponse(mhl.iResponseFromNetwork);
  iRequest(l2l.iRequestFromNetwork);
  l2l.oResponseToNetwork(oResponse);

}

ComputeTile::ComputeTile(const sc_module_name& name, const TileID& id,
                         const tile_parameters_t& params) :
    Tile(name, id),
    mhl("mhl", params),
    l2l("l2l", params),
    icu("icu", params),
    coreToCore("c2c", params),
    coreToMemory("fwdxbar", params),
    dataReturn("dxbar", params),
    instructionReturn("ixbar", params),
    creditReturn("inbound_credits", params),
    bankToMHLRequests("bank_to_mhl_req", params),
    mhlToBankResponses("mhl_to_bank_resp", params),
    bankToL2LResponses("bank_to_l2l_resp", params),
    l2lToBankRequests("l2l_to_bank_req", params.numMemories),
    creditBuffer("credit_buffer", 1, 100),
    invertedClock("inverter") {

  makeComponents(params);
  wireUp(params);

}

