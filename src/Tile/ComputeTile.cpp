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

void ComputeTile::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  if (component.tile != id.tile)
    chip()->storeInstructions(instructions, component);
  else {
    loki_assert(component.isCore());
    cores[component.position]->storeData(instructions);
  }
}

void ComputeTile::storeData(const DataBlock& data) {
  if (data.component().tile != id.tile) {
    chip()->storeData(data);
  }
  else if (data.component().isCore()) {
    cores[data.component().position]->storeData(data.payload());
  }
  else if (data.component().isMemory()) {
    LOKI_ERROR << "storing directly to memory banks is disabled since it cannot be known "
        "which bank the core will access." << endl;
//    assert(data.component().globalMemoryNumber() < memories.size());
//    memories[data.component().globalMemoryNumber()]->storeData(data.payload(),data.position());
  }
}

void ComputeTile::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
  if (component.tile != id.tile || MAGIC_MEMORY)
    chip()->print(component, start, end);
  else if (component.isMemory())
    memories[component.position - CORES_PER_TILE]->print(start, end);
}

void ComputeTile::makeRequest(ComponentID source, ChannelID destination, bool request) {
  loki_assert(!destination.multicast);

  // Find out which signal to write the request to.
  ArbiterRequestSignal *requestSignal;
  ChannelIndex targetBuffer = destination.channel;

  if (source.isCore()) {             // Core to memory
    loki_assert(destination.isMemory());
    requestSignal = &coreToMemRequests[source.position][destination.component.position-CORES_PER_TILE];
  }
  else {                             // Memory to core
    loki_assert(destination.isCore());
    if (targetBuffer < CORE_INSTRUCTION_CHANNELS)
      requestSignal = &instructionReturnRequests[source.position-CORES_PER_TILE][destination.component.position];
    else
      requestSignal = &dataReturnRequests[source.position-CORES_PER_TILE][destination.component.position];
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

  if (source.isCore()) {             // Core to memory
    loki_assert(destination.isMemory());
    grantSignal = &coreToMemGrants[source.position][destination.component.position-CORES_PER_TILE];
  }
  else {                             // Memory/global to core
    loki_assert(destination.isCore());
    if (destination.channel < CORE_INSTRUCTION_CHANNELS)
      grantSignal = &instructionReturnGrants[source.position-CORES_PER_TILE][destination.component.position];
    else
      grantSignal = &dataReturnGrants[source.position-CORES_PER_TILE][destination.component.position];
  }

  return grantSignal->read();
}

Word ComputeTile::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.tile == id.tile && component.isMemory() && !MAGIC_MEMORY)
    return memories[component.position - CORES_PER_TILE]->readWordDebug(addr);
  else
    return chip()->readWordInternal(component, addr);
}

Word ComputeTile::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.tile == id.tile && component.isMemory() && !MAGIC_MEMORY)
    return memories[component.position - CORES_PER_TILE]->readByteDebug(addr);
  else
    return chip()->readByteInternal(component, addr);
}

void ComputeTile::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.tile == id.tile && component.isMemory() && !MAGIC_MEMORY)
    memories[component.position - CORES_PER_TILE]->writeWordDebug(addr, data);
  else
    chip()->writeWordInternal(component, addr, data);
}

void ComputeTile::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.tile == id.tile && component.isMemory() && !MAGIC_MEMORY)
    memories[component.position - CORES_PER_TILE]->writeByteDebug(addr, data);
  else
    chip()->writeByteInternal(component, addr, data);
}

int  ComputeTile::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  if (component.tile != id.tile)
    return chip()->readRegisterInternal(component, reg);
  else {
    loki_assert(component.isCore());
    return cores[component.position]->readRegDebug(reg);
  }
}

bool ComputeTile::readPredicateInternal(const ComponentID& component) const {
  if (component.tile != id.tile)
    return chip()->readPredicateInternal(component);
  else {
    loki_assert(component.isCore());
    return cores[component.position]->readPredReg();
  }
}

void ComputeTile::networkSendDataInternal(const NetworkData& flit) {
  if (flit.channelID().component.tile != id.tile)
    chip()->networkSendDataInternal(flit);
  else {
    loki_assert(flit.channelID().isCore());
    cores[flit.channelID().component.position]->deliverDataInternal(flit);
  }
}

void ComputeTile::networkSendCreditInternal(const NetworkCredit& flit) {
  if (flit.channelID().component.tile != id.tile)
    chip()->networkSendCreditInternal(flit);
  else {
    loki_assert(flit.channelID().isCore());
    cores[flit.channelID().component.position]->deliverCreditInternal(flit);
  }
}

MemoryAddr ComputeTile::getAddressTranslation(MemoryAddr address) const {
  return mhl.getAddressTranslation(address);
}

bool ComputeTile::backedByMainMemory(MemoryAddr address) const {
  return mhl.backedByMainMemory(address);
}

void ComputeTile::makeComponents() {

  TileID tile = id.tile;

  // Initialise the cores of this tile
  for (uint core = 0; core < CORES_PER_TILE; core++) {
    ComponentID coreID(tile, core);

    Core* c = new Core(sc_gen_unique_name("core"), coreID);

    cores.push_back(c);
  }

  // Initialise the memories of this tile
  for (uint mem = 0; mem < MEMS_PER_TILE; mem++) {
    ComponentID memoryID(tile, CORES_PER_TILE + mem);

    MemoryBank* m = new MemoryBank(sc_gen_unique_name("memory"), memoryID);
    m->setBackgroundMemory(&(chip()->mainMemory));

    memories.push_back(m);
  }

}

void ComputeTile::makeSignals() {
  dataToCores.init(dataReturn.oData, "dataToCore");
  dataFromMemory.init(dataReturn.iData, "dataFromMemory");
  instructionsToCores.init(instructionReturn.oData, "instructionsToCores");
  instructionsFromMemory.init(instructionReturn.iData, "instructionsFromMemory");
  requestsToMemory.init(coreToMemory.oData, "requestsToMemory");
  requestsFromCores.init(coreToMemory.iData, "requestsFromCore");
  multicastFromCores.init(coreToCore.iData, "multicastFromCore");
  multicastToCores.init(coreToCore.oData, "multicastToCore");
  readyDataFromCores.init(coreToCore.iReady, "readyDataFromCore");
  readyDataFromMemory.init(coreToMemory.iReady, "readyDataFromMemory");

  creditsToCores.init(cores.size(), "creditToCore");
  creditsFromCores.init(cores.size(), "creditFromCore");
  readyCreditFromCores.init(cores.size(), 1, "readyCreditFromCore");
  globalDataToCores.init(cores.size(), "globalDataToCore");
  globalDataFromCores.init(cores.size(), "globalDataFromCore");

  coreToMemRequests.init(cores.size(), memories.size(), "coreToMemRequest");
  coreToMemGrants.init(cores.size(), memories.size(), "coreToMemGrant");
  for (uint i=0; i<cores.size(); i++)
    for (uint j=0; j<memories.size(); j++)
      coreToMemRequests[i][j].write(NO_REQUEST);

  dataReturnRequests.init(memories.size(), cores.size(), "dataReturnRequest");
  dataReturnGrants.init(memories.size(), cores.size(), "dataReturnGrant");
  instructionReturnRequests.init(memories.size(), cores.size(), "instructionReturnRequest");
  instructionReturnGrants.init(memories.size(), cores.size(), "instructionReturnGrant");
  for (uint i=0; i<memories.size(); i++)
    for (uint j=0; j<cores.size(); j++) {
      dataReturnRequests[i][j].write(NO_REQUEST);
      instructionReturnRequests[i][j].write(NO_REQUEST);
    }

  l2ClaimRequest.init(memories.size(), "l2ClaimRequest");
  l2DelayRequest.init(memories.size(), "l2DelayRequest");
  l2RequestFromMemory.init(memories.size(), "l2RequestFromMemory");
  l2ResponseFromMemory.init(memories.size(), "l2ResponseFromMemory");
}

void ComputeTile::wireUp() {

  for (uint i=0; i<cores.size(); i++) {
    cores[i]->clock(iClock);
    cores[i]->fastClock(fastClock);
    cores[i]->iCredit(creditsToCores[i]);
    cores[i]->iData(dataToCores[i]);
    cores[i]->iDataGlobal(globalDataToCores[i]);
    cores[i]->iInstruction(instructionsToCores[i]);
    cores[i]->iMulticast(multicastToCores[i]); // vector
    cores[i]->oCredit(creditsFromCores[i]);
    cores[i]->oMulticast(multicastFromCores[i]);
    cores[i]->oRequest(requestsFromCores[i]);
    cores[i]->oDataGlobal(globalDataFromCores[i]);
    cores[i]->oReadyCredit(readyCreditFromCores[i][0]);
    cores[i]->oReadyData(readyDataFromCores[i]); // vector
  }

  for (uint i=0; i<memories.size(); i++) {
    memories[i]->iClock(iClock);
    memories[i]->iData(requestsToMemory[i]);
    memories[i]->iRequest(l2RequestToMemory);
    memories[i]->iRequestClaimed(l2RequestClaimed);
    memories[i]->iRequestDelayed(l2RequestDelayed);
    memories[i]->iRequestTarget(l2RequestTarget);
    memories[i]->iResponse(l2ResponseToMemory);
    memories[i]->iResponseTarget(l2ResponseTarget);
    memories[i]->oClaimRequest(l2ClaimRequest[i]);
    memories[i]->oDelayRequest(l2DelayRequest[i]);
    memories[i]->oData(dataFromMemory[i]);
    memories[i]->oInstruction(instructionsFromMemory[i]);
    memories[i]->oReadyForData(readyDataFromMemory[i][0]);
    memories[i]->oRequest(l2RequestFromMemory[i]);
    memories[i]->oResponse(l2ResponseFromMemory[i]);
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
  for (uint i=0; i<CORES_PER_TILE; i++)
    dataReturn.iReady[i](readyDataFromCores[i]);
  dataReturn.iRequest(dataReturnRequests);
  dataReturn.oGrant(dataReturnGrants);

  // Instructions can only go to instruction inputs.
  instructionReturn.clock(slowClock);
  instructionReturn.iData(instructionsFromMemory);
  instructionReturn.oData(instructionsToCores);
  for (uint i=0; i<CORES_PER_TILE; i++)
    for (uint j=0; j<CORE_INSTRUCTION_CHANNELS; j++)
      instructionReturn.iReady[i][j](readyDataFromCores[i][j]);
  instructionReturn.iRequest(instructionReturnRequests);
  instructionReturn.oGrant(instructionReturnGrants);

  dataToRouter.oData(oData);
  dataToRouter.iData(globalDataFromCores);

  dataFromRouter.iData(iData);
  dataFromRouter.oReady(oDataReady);
  dataFromRouter.oData(globalDataToCores);
  for (uint i=0; i<CORES_PER_TILE; i++)
    dataFromRouter.iReady[i](readyDataFromCores[i]);

  creditToRouter.oData(oCredit);
  creditToRouter.iData(creditsFromCores);

  creditFromRouter.iData(iCredit);
  creditFromRouter.oReady(oCreditReady);
  creditFromRouter.iReady(readyCreditFromCores);
  creditFromRouter.oData(creditsToCores);

}

ComputeTile::ComputeTile(const sc_module_name& name, const ComponentID& id) :
    Tile(name, id),
    fastClock("fastClock"),
    slowClock("slowClock"),
    mhl("mhl", id),
    coreToCore("c2c", id),
    coreToMemory("fwdxbar", id),
    dataReturn("dxbar", id),
    instructionReturn("ixbar", id),
    dataToRouter("outbound_data", CORES_PER_TILE),
    dataFromRouter("inbound_data", CORES_PER_TILE, CORE_INPUT_CHANNELS),
    creditToRouter("outbound_credits", CORES_PER_TILE),
    creditFromRouter("inbound_credits", CORES_PER_TILE, 1) {

  makeComponents();
  makeSignals();
  wireUp();

}

ComputeTile::~ComputeTile() {
  for (uint i=0; i<cores.size(); i++)
    delete cores[i];
  for (uint i=0; i<memories.size(); i++)
    delete memories[i];
}

