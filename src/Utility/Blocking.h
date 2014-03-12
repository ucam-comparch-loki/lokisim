/*
 * Blocking.h
 *
 * Class which is capable of blocking (stalling).
 *
 * All components which can block while waiting for something should implement
 * this interface. This includes any component with a network buffer, and
 * pipeline stages which may spend multiple clock cycles on a single
 * instruction.
 *
 * This class allows easier debugging when the system stalls, as we can go
 * directly to the potential problem components.
 *
 *  Created on: 11 Mar 2014
 *      Author: db434
 */

#ifndef BLOCKING_H_
#define BLOCKING_H_

#include <ostream>
#include <vector>

using std::ostream;
using std::vector;

class Blocking {

public:

  // Constructor
  Blocking();
  virtual ~Blocking();

  // Print all observed issues in the system.
  static void reportProblems(ostream& os);

protected:

  // Report any conditions which may cause stalling. Must be implemented in
  // every subclass.
  virtual void reportStalls(ostream& os) = 0;

private:

  // A list of all components which may have problems to report.
  static vector<Blocking*> components;

};

#endif /* BLOCKING_H_ */
