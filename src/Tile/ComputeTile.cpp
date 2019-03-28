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

void ComputeTile::makeSignals() {
  dataToCores.init("dataToCore", dataReturn.oData);
  dataFromMemory.init("dataFromMemory", dataReturn.iData);
  instructionsToCores.init("instructionsToCores", instructionReturn.oData);
  instructionsFromMemory.init("instructionsFromMemory", instructionReturn.iData);
  multicastFromCores.init("multicastFromCore", coreToCore.iData);
  multicastToCores.init("multicastToCore", coreToCore.oData);
  readyDataFromCores.init("readyDataFromCore", coreToCore.iReady);

  creditsToCores.init("creditToCore", cores.size());
  creditsFromCores.init("creditFromCore", cores.size());
  readyCreditFromCores.init("readyCreditFromCore", cores.size(), 1);
  globalDataToCores.init("globalDataToCore", cores.size());
  globalDataFromCores.init("globalDataFromCore", cores.size());

  dataReturnRequests.init("dataReturnRequest", memories.size(), cores.size());
  dataReturnGrants.init("dataReturnGrant", memories.size(), cores.size());
  instructionReturnRequests.init("instructionReturnRequest", memories.size(), cores.size());
  instructionReturnGrants.init("instructionReturnGrant", memories.size(), cores.size());
  for (uint i=0; i<memories.size(); i++)
    for (uint j=0; j<cores.size(); j++) {
      dataReturnRequests[i][j].write(NO_REQUEST);
      instructionReturnRequests[i][j].write(NO_REQUEST);
    }

  l2ClaimRequest.init("l2ClaimRequest", memories.size());
  l2DelayRequest.init("l2DelayRequest", memories.size());
  l2RequestFromMemory.init("l2RequestFromMemory", memories.size());
  l2ResponseFromMemory.init("l2ResponseFromMemory", memories.size());
}

void ComputeTile::wireUp() {

  for (uint i=0; i<cores.size(); i++) {
    cores[i].clock(clock);
    cores[i].iCredit(creditsToCores[i]);
    cores[i].iData(dataToCores[i]);
    cores[i].iDataGlobal(globalDataToCores[i]);
    cores[i].iInstruction(instructionsToCores[i]);
    cores[i].iMulticast(multicastToCores[i]); // vector
    cores[i].oCredit(creditsFromCores[i]);
    cores[i].oMulticast(multicastFromCores[i]);
    cores[i].oDataGlobal(globalDataFromCores[i]);
    cores[i].oReadyCredit(readyCreditFromCores[i][0]);
    cores[i].oReadyData(readyDataFromCores[i]); // vector
  }

  for (uint i=0; i<memories.size(); i++) {
    memories[i].iClock(clock);
    memories[i].iRequest(l2RequestToMemory);
    memories[i].iRequestClaimed(l2RequestClaimed);
    memories[i].iRequestDelayed(l2RequestDelayed);
    memories[i].iRequestTarget(l2RequestTarget);
    memories[i].iResponse(l2ResponseToMemory);
    memories[i].iResponseTarget(l2ResponseTarget);
    memories[i].oClaimRequest(l2ClaimRequest[i]);
    memories[i].oDelayRequest(l2DelayRequest[i]);
    memories[i].oData(dataFromMemory[i]);
    memories[i].oInstruction(instructionsFromMemory[i]);
    memories[i].oCoreDataRequest(dataReturnRequests[i]); // vector
    memories[i].iCoreDataGrant(dataReturnGrants[i]); // vector
    memories[i].oCoreInstRequest(instructionReturnRequests[i]); // vector
    memories[i].iCoreInstGrant(instructionReturnGrants[i]); // vector
    memories[i].oRequest(l2RequestFromMemory[i]);
    memories[i].oResponse(l2ResponseFromMemory[i]);
  }

  mhl.clock(clock);
  mhl.oRequestToNetwork(oRequest);
  mhl.oReadyForRequest(oRequestReady);
  mhl.iRequestFromNetwork(iRequest);
  mhl.oResponseToNetwork(oResponse);
  mhl.oReadyForResponse(oResponseReady);
  mhl.iResponseFromNetwork(iResponse);
  mhl.iClaimRequest(l2ClaimRequest);
  mhl.iDelayRequest(l2DelayRequest);
  mhl.iRequestFromBanks(l2RequestFromMemory);
  mhl.iResponseFromBanks(l2ResponseFromMemory);
  mhl.oRequestClaimed(l2RequestClaimed);
  mhl.oRequestDelayed(l2RequestDelayed);
  mhl.oRequestTarget(l2RequestTarget);
  mhl.oRequestToBanks(l2RequestToMemory);
  mhl.oResponseTarget(l2ResponseTarget);
  mhl.oResponseToBanks(l2ResponseToMemory);

  // Each subnetwork contains arbiters, and the outputs of the networks
  // are themselves arbitrated. Use the slow clock because the memory sends
  // its data on the negative edge, and the core may need the time to compute
  // which memory bank it is sending to.

  coreToCore.iData(multicastFromCores);
  coreToCore.oData(multicastToCores);
  coreToCore.iReady(readyDataFromCores);

  coreToMemory.clock(clock);
  for (uint i=0; i<cores.size(); i++)
    coreToMemory.inputs[i](cores[i].oRequest);
  for (uint i=0; i<memories.size(); i++)
    coreToMemory.outputs[i](memories[i].iData);

  // Data can go to any buffer (including instructions).
  dataReturn.clock(slowClock);
  dataReturn.iData(dataFromMemory);
  dataReturn.oData(dataToCores);
  dataReturn.iReady(readyDataFromCores);
  dataReturn.iRequest(dataReturnRequests);
  dataReturn.oGrant(dataReturnGrants);

  // Instructions can only go to instruction inputs.
  instructionReturn.clock(slowClock);
  instructionReturn.iData(instructionsFromMemory);
  instructionReturn.oData(instructionsToCores);
  for (uint i=0; i<readyDataFromCores.size(); i++)
    for (uint j=0; j<Core::numInstructionChannels; j++)
      instructionReturn.iReady[i][j](readyDataFromCores[i][j]);
  instructionReturn.iRequest(instructionReturnRequests);
  instructionReturn.oGrant(instructionReturnGrants);

  dataToRouter.oData(oData);
  dataToRouter.iData(globalDataFromCores);

  dataFromRouter.iData(iData);
  dataFromRouter.oReady(oDataReady);
  dataFromRouter.oData(globalDataToCores);
  dataFromRouter.iReady(readyDataFromCores);

  creditToRouter.oData(oCredit);
  creditToRouter.iData(creditsFromCores);

  creditFromRouter.iData(iCredit);
  creditFromRouter.oReady(oCreditReady);
  creditFromRouter.iReady(readyCreditFromCores);
  creditFromRouter.oData(creditsToCores);

}

ComputeTile::ComputeTile(const sc_module_name& name, const TileID& id,
                         const tile_parameters_t& params) :
    Tile(name, id),
    slowClock("slowClock"),
    mhl("mhl", params),
    coreToCore("c2c", params),
    coreToMemory("fwdxbar", params),
    dataReturn("dxbar", params),
    instructionReturn("ixbar", params),
    dataToRouter("outbound_data", params.numCores),
    dataFromRouter("inbound_data", params.numCores, params.core.numInputChannels),
    creditToRouter("outbound_credits", params.numCores),
    creditFromRouter("inbound_credits", params.numCores, 1) {

  makeComponents(params);
  makeSignals();
  wireUp();

}

