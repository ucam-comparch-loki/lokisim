/*
 * BlockingInterface.cpp
 *
 *  Created on: 11 Mar 2014
 *      Author: db434
 */

#include "BlockingInterface.h"

std::vector<BlockingInterface*> BlockingInterface::components;

// Register this component as one which may cause problems.
BlockingInterface::BlockingInterface() {
  components.push_back(this);
}

BlockingInterface::~BlockingInterface() {
  // Nothing
}

void BlockingInterface::reportProblems(ostream& os) {
  os << "\nReport of potential blockages:\n";
  for (unsigned int i=0; i<components.size(); i++) {
    components[i]->reportStalls(os);
  }
  os << "End of blockage report.\n";
}

