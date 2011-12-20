/*
 * NewBus.h
 *
 * Performs exactly the same job as a pair of sc_signals, but allows extra
 * monitoring to be done (e.g. count bits which switched).
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#ifndef NEWBUS_H_
#define NEWBUS_H_

#include "../../Component.h"
#include "../NetworkTypedefs.h"

class NewBus: public Component {

//==============================//
// Ports
//==============================//

public:

  DataInput  dataIn;
  DataOutput dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(NewBus);
  NewBus(const sc_module_name& name, const ComponentID& ID, int numOutputPorts,
         Dimension size);

//==============================//
// Methods
//==============================//

public:

  int numOutputs() const;

protected:

  // FIXME: These seem to be unnecessary overhead - connect the signals directly
  // to the destinations?
  virtual void receivedData();
  virtual void receivedAck();

private:

  // Compute how many bits switched, and call the appropriate instrumentation
  // methods.
  void computeSwitching();

//==============================//
// Local state
//==============================//

private:

  const int outputs;
  const Dimension size;

  // Store the previous value, so we can compute how many bits change when a
  // new value arrives.
  DataType lastData;

  // The cycle during which data was last written to the bus. Data should be
  // written at most once per clock cycle.
  unsigned long lastWriteTime;

};

#endif /* NEWBUS_H_ */
