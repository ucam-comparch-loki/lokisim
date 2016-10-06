/*
 * ComputeTile.cpp
 *
 *  Created on: 4 Oct 2016
 *      Author: db434
 */

#include <sstream>

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
    cores[data.component().position]->storeData(data.payload(), data.position());
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

void ComputeTile::makeComponents() {

  TileID tile = id.tile;

  // Initialise the cores of this tile
  for (uint core = 0; core < CORES_PER_TILE; core++) {
    ComponentID coreID(tile, core);

    std::stringstream name;
    name << "core_" << core;

    Core* c = new Core(name.str().c_str(), coreID, &localNetwork);

    cores.push_back(c);
  }

  // Initialise the memories of this tile
  for (uint mem = 0; mem < MEMS_PER_TILE; mem++) {
    ComponentID memoryID(tile, CORES_PER_TILE + mem);

    std::stringstream name;
    name << "memory_" << mem;

    MemoryBank* m = new MemoryBank(name.str().c_str(), memoryID, mem);
    m->setLocalNetwork(&localNetwork);
    m->setBackgroundMemory(&(chip()->mainMemory));

    memories.push_back(m);
  }

}

void ComputeTile::makeSignals() {
  creditsToCores.init(cores.size());
  creditsFromCores.init(cores.size());
  readyCreditFromCores.init(cores.size(), 1);
  globalDataToCores.init(cores.size());
  globalDataFromCores.init(cores.size());

  claimRequest.init(memories.size());
  requestFromMemory.init(memories.size());
  responseFromMemory.init(memories.size());

  dataToComponents.init(cores.size() + memories.size());
  dataFromComponents.init(cores.size() + memories.size());
  readyDataFromComponents.init(cores.size() + memories.size());

  for (uint i=0; i<cores.size(); i++) {
    dataToComponents[i].init(cores[i]->iData);
    dataFromComponents[i].init(cores[i]->oData);
    readyDataFromComponents[i].init(cores[i]->oReadyData);
  }

  for (uint i=cores.size(); i<cores.size()+memories.size(); i++) {
    dataToComponents[i].init(1);
    dataFromComponents[i].init(1);
    readyDataFromComponents[i].init(1);
  }
}

void ComputeTile::wireUp() {

  for (uint i=0; i<cores.size(); i++) {
    cores[i]->clock(clock);
    cores[i]->fastClock(fastClock);
    cores[i]->iCredit(creditsToCores[i]);
    cores[i]->iData(dataToComponents[i]); // vector
    cores[i]->iDataGlobal(globalDataToCores[i]);
    cores[i]->oCredit(creditsFromCores[i]);
    cores[i]->oData(dataFromComponents[i]); // vector
    cores[i]->oDataGlobal(globalDataFromCores[i]);
    cores[i]->oReadyCredit(readyCreditFromCores[i][0]);
    cores[i]->oReadyData(readyDataFromComponents[i]); // vector
  }

  for (uint i=0; i<memories.size(); i++) {
    memories[i]->iClock(clock);
    memories[i]->iData(dataToComponents[i+CORES_PER_TILE][0]);
    memories[i]->iRequest(requestToMemory);
    memories[i]->iRequestClaimed(requestClaimed);
    memories[i]->iRequestTarget(requestTarget);
    memories[i]->iResponse(responseToMemory);
    memories[i]->iResponseTarget(responseTarget);
    memories[i]->oClaimRequest(claimRequest[i]);
    memories[i]->oData(dataFromComponents[i+CORES_PER_TILE][0]);
    memories[i]->oReadyForData(readyDataFromComponents[i+CORES_PER_TILE][0]);
    memories[i]->oRequest(requestFromMemory[i]);
    memories[i]->oResponse(responseFromMemory[i]);
  }

  localNetwork.clock(clock);
  localNetwork.fastClock(fastClock);
  localNetwork.slowClock(slowClock);
  localNetwork.iData(dataFromComponents);
  localNetwork.iReady(readyDataFromComponents);
  localNetwork.oData(dataToComponents);

  mhl.clock(clock);
  mhl.oRequestToNetwork(oRequest);
  mhl.oReadyForRequest(oRequestReady);
  mhl.iRequestFromNetwork(iRequest);
  mhl.oResponseToNetwork(oResponse);
  mhl.oReadyForResponse(oResponseReady);
  mhl.iResponseFromNetwork(iResponse);
  mhl.iClaimRequest(claimRequest);
  mhl.iRequestFromBanks(requestFromMemory);
  mhl.iResponseFromBanks(responseFromMemory);
  mhl.oRequestClaimed(requestClaimed);
  mhl.oRequestTarget(requestTarget);
  mhl.oRequestToBanks(requestToMemory);
  mhl.oResponseTarget(responseTarget);
  mhl.oResponseToBanks(responseToMemory);
  mhl.iResponseFromBM(iResponseFromMainMemory);
  mhl.oRequestToBM(oRequestToMainMemory);

  dataToRouter.oData(oData);
  dataToRouter.iData(globalDataFromCores);

  dataFromRouter.iData(iData);
  dataFromRouter.oReady(oDataReady);
  dataFromRouter.oData(globalDataToCores);
  for (uint i=0; i<cores.size(); i++)  // Only connect the cores.
    dataFromRouter.iReady[i](readyDataFromComponents[i]);

  creditToRouter.oData(oCredit);
  creditToRouter.iData(creditsFromCores);

  creditFromRouter.iData(iData);
  creditFromRouter.oReady(oCreditReady);
  creditFromRouter.iReady(readyCreditFromCores);
  creditFromRouter.oData(creditsToCores);

}

ComputeTile::ComputeTile(const sc_module_name& name, const ComponentID& id) :
    Tile(name, id),
    mhl("mhl", id),
    localNetwork("local", id),
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

