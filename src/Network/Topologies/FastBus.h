/*
 * FastBus.h
 *
 *  Created on: 29 May 2011
 *      Author: afjk2
 */

#ifndef FASTBUS_H_
#define FASTBUS_H_

#include "../Network.h"
#include "../../Datatype/AddressedWord.h"
#include "../NetworkTypedefs.h"

class FastBus: public Network {

//==============================//
// Ports
//==============================//

public:

//  Inherited from Network:
//
//   sc_in<bool>   clock;
//
//   DataInput    *dataIn;
//   ReadyInput   *validDataIn;
//   ReadyOutput  *ackDataIn;
//
//   DataOutput   *dataOut;
//   ReadyOutput  *validDataOut;
//   ReadyInput   *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FastBus);

  FastBus(sc_module_name name, const ComponentID& ID, int numOutputPorts, HierarchyLevel level, Dimension size);

//==============================//
// Methods
//==============================//

private:

  void busProcess();

  // Compute how many bits switched, and call the appropriate instrumentation
  // methods.
  void computeSwitching();

//==============================//
// Local state
//==============================//

private:

  enum BusState {
	  STATE_INIT,
	  STATE_STANDBY,
	  STATE_OUTPUT_VALID,
	  STATE_ACKNOWLEDGED
  };

  // Store the previous value, so we can compute how many bits change when a
  // new value arrives.
  sc_signal<DataType> lastData;

  BusState execState;
  unsigned long long execCycle;
  PortIndex output;

  sc_signal<int> triggerSignal;
};

#endif /* FASTBUS_H_ */
