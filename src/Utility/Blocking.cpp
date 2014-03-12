/*
 * Blocking.cpp
 *
 *  Created on: 11 Mar 2014
 *      Author: db434
 */

#include "Blocking.h"

std::vector<Blocking*> Blocking::components;

// Register this component as one which may cause problems.
Blocking::Blocking() {
  components.push_back(this);
}

Blocking::~Blocking() {
  // Nothing
}

void Blocking::reportProblems(ostream& os) {
  os << "\nReport of potential blockages:\n";
  for (unsigned int i=0; i<components.size(); i++) {
    components[i]->reportStalls(os);
  }
  os << "End of blockage report.\n";
}

