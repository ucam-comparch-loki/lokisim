/*
 * Bus.h
 *
 *  Created on: 31 Mar 2011
 *      Author: db434
 */

#ifndef BUS_H_
#define BUS_H_

#include "../Network.h"
#include "../../Datatype/AddressedWord.h"
#include "../NetworkTypedefs.h"

class Bus: public Network {

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

  SC_HAS_PROCESS(Bus);

  Bus(sc_module_name name, const ComponentID& ID, int numOutputPorts,
      HierarchyLevel level, Dimension size);

//==============================//
// Methods
//==============================//

protected:

  virtual void mainLoop();
  virtual void receivedData();
  virtual void receivedCredit(PortIndex output);

private:

  // Compute how many bits switched, and call the appropriate instrumentation
  // methods.
  void computeSwitching();

//==============================//
// Local state
//==============================//

private:

  // Store the previous value, so we can compute how many bits change when a
  // new value arrives.
  DataType lastData;

};

#endif /* BUS_H_ */
