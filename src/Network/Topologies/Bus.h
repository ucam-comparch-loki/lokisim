/*
 * Bus.h
 *
 * A simple single-source, single-destination bus.
 *
 * Any received data is sent on as soon as it is received. It is the
 * responsibility of the source to send a maximum of one word per cycle.
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
//   DataOutput   *dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Bus);

  Bus(const sc_module_name& name, const ComponentID& ID, int numOutputPorts,
      HierarchyLevel level, Dimension size, int firstOutput=0);

//==============================//
// Methods
//==============================//

protected:

  // Main loop responsible for sending and receiving data, and dealing with
  // acknowledgements.
  virtual void busLoop();

//==============================//
// Local state
//==============================//

protected:

  enum BusState {WAITING_FOR_DATA, WAITING_FOR_ACK};

  BusState state;

  // The output port we have sent data on.
  PortIndex outputUsed;

private:

  // The cycle during which data was last written to the bus. Data should be
  // written at most once per clock cycle.
  unsigned long lastWriteTime;

};

#endif /* BUS_H_ */
