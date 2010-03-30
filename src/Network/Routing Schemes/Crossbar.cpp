/*
 * Crossbar.cpp
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#include "Crossbar.h"

void Crossbar::route(sc_in<AddressedWord> *inputs, sc_out<Word> *outputs,
                     int length, std::vector<bool>& sent) {

  for(int i=0; i<length; i++) {

    AddressedWord aw = inputs[i].read();
    short chID = aw.getChannelID();

    // If we haven't already sent to this output, and the input is new, send.
    if(!sent[chID] && inputs[i].event()) {

      outputs[chID].write(aw.getPayload());   // Write the data
      sent[chID] = true;                      // Stop anyone else from writing

      // Just some complicated-looking debug output
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

  // Clear the vector for next time.
  for(unsigned int i=0; i<sent.size(); i++) sent[i] = false;

}

Crossbar::Crossbar() {

}

Crossbar::~Crossbar() {

}
