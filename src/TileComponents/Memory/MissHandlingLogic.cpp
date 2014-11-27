/*
 * MissHandlingLogic.cpp
 *
 *  Created on: 8 Oct 2014
 *      Author: db434
 */

#include "MissHandlingLogic.h"

MissHandlingLogic::MissHandlingLogic(const sc_module_name& name, ComponentID id) :
    Component(name, id),
    inputMux("mux", MEMS_PER_TILE) {

  iRequestFromBanks.init(MEMS_PER_TILE);

  state = MHL_READY;

  for (uint i=0; i<iRequestFromBanks.length(); i++)
    inputMux.iData[i](iRequestFromBanks[i]);
  inputMux.oData(muxOutput);

  // TODO: need ability to hold the same input of the mux, to allow entire
  // packets through.
}

