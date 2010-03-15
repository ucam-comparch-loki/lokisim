/*
 * Crossbar.cpp
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#include "Crossbar.h"

void Crossbar::route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs,
                     int length) {

  for(int i=0; i<length; i++) {
    if(inputs[i].event() /*&& haven't already written to this output*/) {
      AddressedWord aw = inputs[i].read();
      short chID = aw.getChannelID();
      outputs[chID].write(aw.getPayload());

      if(DEBUG) {
        bool fromOutputs = (length == NUM_CLUSTER_OUTPUTS*COMPONENTS_PER_TILE);
        std::string in = fromOutputs ? "input " : "output ";
        std::string out = fromOutputs ? "output " : "input ";
        int inChans = fromOutputs ? NUM_CLUSTER_OUTPUTS : NUM_CLUSTER_INPUTS;
        int outChans = fromOutputs ? NUM_CLUSTER_INPUTS : NUM_CLUSTER_OUTPUTS;

        cout << "Network sent " << aw.getPayload() <<
             " from channel "<<i<<" (comp "<<i/inChans<<", "<<out<<i%inChans<<
             ") to channel "<<chID<<" (comp "<<chID/outChans<<", "<<in<<chID%outChans
             << ")" << endl;
      }
    }
  }

}

Crossbar::Crossbar() {

}

Crossbar::~Crossbar() {

}
