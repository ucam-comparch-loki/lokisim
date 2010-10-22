/*
 * Crossbar.cpp
 *
 *  Created on: 15 Mar 2010
 *      Author: db434
 */

#include "Crossbar.h"
#include "math.h"

void Crossbar::route(input_port inputs[],
                     output_port outputs[],
                     int length,
                     std::vector<bool>& sent,
                     bool instrumentation) {

  for(int i=0; i<length; i++) {   // Is round-robin required here?

    // If the input is new, see if we can send
    if(inputs[i].event()) {
      AddressedWord aw = inputs[i].read();
      short chID = aw.channelID();

      // If we haven't already sent to this output, send
      if(!sent[chID]) {
        outputs[chID].write(aw);   // Write the data
        sent[chID] = true;                      // Stop anyone else from writing

        // TODO: extract instrumentation to base class
        if(instrumentation) {
          int startID = i/NUM_CLUSTER_OUTPUTS;
          int endID   = chID/NUM_CLUSTER_INPUTS;
          Instrumentation::networkTraffic(startID, endID, distance(startID,endID));

          if(DEBUG) printMessage(length, aw, i, chID);
        }
      }
      // If we have already sent to this output, deny the request
      else {
        continue;
      }
    }

  }// end for

  // Clear the vector for next time.
  for(uint i=0; i<sent.size(); i++) sent[i] = false;

}

double Crossbar::distance(int startID, int endID) {
  // Assuming a tile is square, find its width.
  static const double tileWidth = sqrt(COMPONENTS_PER_TILE);
  static const int roundedWidth = ceil(tileWidth);
  static const double distThruCrossbar = 1; // Fairly arbitrary for now

  int startRow = startID  / roundedWidth;
  int startCol = startID  % roundedWidth;
  int endRow   = endID    / roundedWidth;
  int endCol   = endID    % roundedWidth;

  static double crossbarXPos = tileWidth/2 - 0.5;
  static double crossbarYPos = tileWidth/2 - 0.5;

  double startToCentre = abs(startRow-crossbarYPos) + abs(startCol-crossbarXPos);
  double centreToEnd   = abs(endRow-crossbarYPos) + abs(endCol-crossbarXPos);

  return startToCentre + centreToEnd + distThruCrossbar;
}

/* Print a message saying where the data was sent from and to. */
void Crossbar::printMessage(uint length, AddressedWord data, int from, int to) {

  bool fromOutputs = (length == NUM_CLUSTER_OUTPUTS*COMPONENTS_PER_TILE);
  std::string in   = fromOutputs ? "input " : "output ";
  std::string out  = fromOutputs ? "output " : "input ";
  int inChans      = fromOutputs ? NUM_CLUSTER_OUTPUTS : NUM_CLUSTER_INPUTS;
  int outChans     = fromOutputs ? NUM_CLUSTER_INPUTS  : NUM_CLUSTER_OUTPUTS;

  cout << "Network sent " << data.payload() <<
       " from channel "<<from<<" (comp "<<from/inChans<<", "<<out<<from%inChans<<
       ") to channel "<<to<<" (comp "<<to/outChans<<", "<<in<<to%outChans
       << ")" << endl;

}

Crossbar::Crossbar() {

}

Crossbar::~Crossbar() {

}
