/*
 * NewBus.h
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

   DataInput    dataIn;
   ReadyInput   validDataIn;
   ReadyOutput  ackDataIn;

   DataOutput   dataOut;
   ReadyOutput  validDataOut;

   // Received acknowledgements are the only ports which require one port per
   // destination. The same data and valid signal can go to all destinations.
   ReadyInput  *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(NewBus);
  NewBus(const sc_module_name& name, const ComponentID& ID, int numOutputPorts,
         Dimension size);
  virtual ~NewBus();

//==============================//
// Methods
//==============================//

public:

  int numOutputs() const;

protected:

  virtual void receivedData();
  virtual void receivedAck(int output);

  // Triggered whenever receivedAllAcks is notified.
  virtual void sendAck();

private:

  // Compute how many bits switched, and call the appropriate instrumentation
  // methods.
  void computeSwitching();

//==============================//
// Local state
//==============================//

protected:

  // Event which is notified when all acknowledgements have been received.
  sc_event receivedAllAcks, allAcksDeasserted;

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
