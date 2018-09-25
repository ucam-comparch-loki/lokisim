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
  return id.position;
}

uint ComputeTile::coreIndex(ComponentID id) const {
  return id.position;
}

uint ComputeTile::memoryIndex(ComponentID id) const {
  return id.position - numCores();
}

uint ComputeTile::globalCoreIndex(ComponentID id) const {
  return chip().globalCoreIndex(id);
}

uint ComputeTile::globalMemoryIndex(ComponentID id) const {
  return chip().globalMemoryIndex(id);
}

void ComputeTile::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  if (component.tile != id.tile)
    chip().storeInstructions(instructions, component);
  else {
    loki_assert(isCore(component));
    cores[coreIndex(component)].storeData(instructions);
  }
}

void ComputeTile::storeData(const DataBlock& data) {
  if (data.component().tile != id.tile) {
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
  if (component.tile != id.tile || MAGIC_MEMORY)
    chip().print(component, start, end);
  else if (isMemory(component))
    memories[memoryIndex(component)].print(start, end);
}

void ComputeTile::makeRequest(ComponentID source, ChannelID destination, bool request) {
  loki_assert(!destination.multicast);

  // Find out which signal to write the request to.
  ArbiterRequestSignal *requestSignal;
  ChannelIndex targetBuffer = destination.channel;

  if (isCore(source)) {              // Core to memory
    loki_assert(isMemory(destination.component));
    requestSignal = &coreToMemRequests[coreIndex(source)][memoryIndex(destination.component)];
  }
  else {                             // Memory to core
    loki_assert(isCore(destination.component));
    if (targetBuffer < Core::numInstructionChannels)
      requestSignal = &instructionReturnRequests[memoryIndex(source)][coreIndex(destination.component)];
    else
      requestSignal = &dataReturnRequests[memoryIndex(source)][coreIndex(destination.component)];
  }

  // Send the request.
  if (request)
    requestSignal->write(targetBuffer);
  else
    requestSignal->write(NO_REQUEST);
}

bool ComputeTile::requestGranted(ComponentID source, ChannelID destination) const {
  assert(!destination.multicast);

  ArbiterGrantSignal   *grantSignal;

  if (isCore(source)) {              // Core to memory
    loki_assert(isMemory(destination.component));
    grantSignal = &coreToMemGrants[coreIndex(source)][memoryIndex(destination.component)];
  }
  else {                             // Memory/global to core
    loki_assert(isCore(destination.component));
    if (destination.channel < Core::numInstructionChannels)
      grantSignal = &instructionReturnGrants[memoryIndex(source)][coreIndex(destination.component)];
    else
      grantSignal = &dataReturnGrants[memoryIndex(source)][coreIndex(destination.component)];
  }

  return grantSignal->read();
}

Word ComputeTile::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.tile == id.tile && isMemory(component) && !MAGIC_MEMORY)
    return memories[memoryIndex(component)].readWordDebug(addr);
  else
    return chip().readWordInternal(component, addr);
}

Word ComputeTile::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.tile == id.tile && isMemory(component) && !MAGIC_MEMORY)
    return memories[memoryIndex(component)].readByteDebug(addr);
  else
    return chip().readByteInternal(component, addr);
}

void ComputeTile::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.tile == id.tile && isMemory(component) && !MAGIC_MEMORY)
    memories[memoryIndex(component)].writeWordDebug(addr, data);
  else
    chip().writeWordInternal(component, addr, data);
}

void ComputeTile::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.tile == id.tile && isMemory(component) && !MAGIC_MEMORY)
    memories[memoryIndex(component)].writeByteDebug(addr, data);
  else
    chip().writeByteInternal(component, addr, data);
}

int  ComputeTile::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  if (component.tile != id.tile)
    return chip().readRegisterInternal(component, reg);
  else {
    loki_assert(isCore(component));
    return cores[coreIndex(component)].readRegDebug(reg);
  }
}

bool ComputeTile::readPredicateInternal(const ComponentID& component) const {
  if (component.tile != id.tile)
    return chip().readPredicateInternal(component);
  else {
    loki_assert(isCore(component));
    return cores[coreIndex(component)].readPredReg();
  }
}

void ComputeTile::networkSendDataInternal(const NetworkData& flit) {
  if (flit.channelID().component.tile != id.tile)
    chip().networkSendDataInternal(flit);
  else {
    loki_assert(isCore(flit.channelID().component));
    cores[coreIndex(flit.channelID().component)].deliverDataInternal(flit);
  }
}

void ComputeTile::networkSendCreditInternal(const NetworkCredit& flit) {
  if (flit.channelID().component.tile != id.tile)
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

  TileID tile = id.tile;

  // Initialise the cores of this tile
  for (uint core = 0; core < params.numCores; core++) {
    ComponentID coreID(tile, core);

    Core* c = new Core(sc_gen_unique_name("core"), coreID, params.core,
                       params.mcastNetOutputs());

    cores.push_back(c);
  }

  // Initialise the memories of this tile
  for (uint mem = 0; mem < params.numMemories; mem++) {
    ComponentID memoryID(tile, params.numCores + mem);

    MemoryBank* m = new MemoryBank(sc_gen_unique_name("memory"), memoryID,
                                   params.numMemories, params.memory);
    m->setBackgroundMemory(&(chip().mainMemory));

    memories.push_back(m);
  }

}

void ComputeTile::makeSignals() {
  dataToCores.init("dataToCore", dataReturn.oData);
  dataFromMemory.init("dataFromMemory", dataReturn.iData);
  instructionsToCores.init("instructionsToCores", instructionReturn.oData);
  instructionsFromMemory.init("instructionsFromMemory", instructionReturn.iData);
  requestsToMemory.init("requestsToMemory", coreToMemory.oData);
  requestsFromCores.init("requestsFromCore", coreToMemory.iData);
  multicastFromCores.init("multicastFromCore", coreToCore.iData);
  multicastToCores.init("multicastToCore", coreToCore.oData);
  readyDataFromCores.init("readyDataFromCore", coreToCore.iReady);
  readyDataFromMemory.init("readyDataFromMemory", coreToMemory.iReady);

  creditsToCores.init("creditToCore", cores.size());
  creditsFromCores.init("creditFromCore", cores.size());
  readyCreditFromCores.init("readyCreditFromCore", cores.size(), 1);
  globalDataToCores.init("globalDataToCore", cores.size());
  globalDataFromCores.init("globalDataFromCore", cores.size());

  coreToMemRequests.init("coreToMemRequest", cores.size(), memories.size());
  coreToMemGrants.init("coreToMemGrant", cores.size(), memories.size());
  for (uint i=0; i<cores.size(); i++)
    for (uint j=0; j<memories.size(); j++)
      coreToMemRequests[i][j].write(NO_REQUEST);

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
    cores[i].clock(iClock);
    cores[i].fastClock(fastClock);
    cores[i].iCredit(creditsToCores[i]);
    cores[i].iData(dataToCores[i]);
    cores[i].iDataGlobal(globalDataToCores[i]);
    cores[i].iInstruction(instructionsToCores[i]);
    cores[i].iMulticast(multicastToCores[i]); // vector
    cores[i].oCredit(creditsFromCores[i]);
    cores[i].oMulticast(multicastFromCores[i]);
    cores[i].oRequest(requestsFromCores[i]);
    cores[i].oDataGlobal(globalDataFromCores[i]);
    cores[i].oReadyCredit(readyCreditFromCores[i][0]);
    cores[i].oReadyData(readyDataFromCores[i]); // vector
  }

  for (uint i=0; i<memories.size(); i++) {
    memories[i].iClock(iClock);
    memories[i].iData(requestsToMemory[i]);
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
    memories[i].oReadyForData(readyDataFromMemory[i][0]);
    memories[i].oRequest(l2RequestFromMemory[i]);
    memories[i].oResponse(l2ResponseFromMemory[i]);
  }

  mhl.clock(iClock);
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

  coreToCore.clock(slowClock);
  coreToCore.iData(multicastFromCores);
  coreToCore.oData(multicastToCores);
  coreToCore.iReady(readyDataFromCores);

  coreToMemory.clock(slowClock);
  coreToMemory.iData(requestsFromCores);
  coreToMemory.oData(requestsToMemory);
  coreToMemory.iReady(readyDataFromMemory);
  coreToMemory.iRequest(coreToMemRequests);
  coreToMemory.oGrant(coreToMemGrants);

  // Data can go to any buffer (including instructions).
  dataReturn.clock(slowClock);
  dataReturn.iData(dataFromMemory);
  dataReturn.oData(dataToCores);
  for (uint i=0; i<cores.size(); i++)
    dataReturn.iReady[i](readyDataFromCores[i]);
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
  for (uint i=0; i<cores.size(); i++)
    dataFromRouter.iReady[i](readyDataFromCores[i]);

  creditToRouter.oData(oCredit);
  creditToRouter.iData(creditsFromCores);

  creditFromRouter.iData(iCredit);
  creditFromRouter.oReady(oCreditReady);
  creditFromRouter.iReady(readyCreditFromCores);
  creditFromRouter.oData(creditsToCores);

}

ComputeTile::ComputeTile(const sc_module_name& name, const ComponentID& id,
                         const tile_parameters_t& params) :
    Tile(name, id),
    fastClock("fastClock"),
    slowClock("slowClock"),
    mhl("mhl", id, params),
    coreToCore("c2c", id, params),
    coreToMemory("fwdxbar", id, params),
    dataReturn("dxbar", id, params),
    instructionReturn("ixbar", id, params),
    dataToRouter("outbound_data", params.numCores),
    dataFromRouter("inbound_data", params.numCores, params.core.numInputChannels),
    creditToRouter("outbound_credits", params.numCores),
    creditFromRouter("inbound_credits", params.numCores, 1) {

  makeComponents(params);
  makeSignals();
  wireUp();

}

