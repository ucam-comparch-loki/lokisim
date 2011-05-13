/*
 * UnclockedNetwork.h
 *
 * A network which allows data through as soon as it arrives, rather than
 * waiting for a clock edge.
 *
 * This class acts as a wrapper for any other Network.
 *
 *  Created on: 27 Apr 2011
 *      Author: db434
 */

#ifndef UNCLOCKEDNETWORK_H_
#define UNCLOCKEDNETWORK_H_

#include "../Component.h"
#include "NetworkTypedefs.h"

class Network;

// Should UnclockedNetwork extend Network instead? It doesn't have a clock port.
class UnclockedNetwork: public Component {

//==============================//
// Ports
//==============================//

public:

  DataInput    *dataIn;
  ReadyInput   *validDataIn;
  ReadyOutput  *ackDataIn;
  DataOutput   *dataOut;
  ReadyOutput  *validDataOut;
  ReadyInput   *ackDataOut;

//==============================//
// Methods
//==============================//

public:

  // Provide access to the underlying network, in case there are specific
  // ports/methods which are needed.
  Network* network() const;

private:

  void dataArrived();
  void generateClock();

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(UnclockedNetwork);
  UnclockedNetwork(sc_module_name name, Network* network);
  virtual ~UnclockedNetwork();

//==============================//
// Local state
//==============================//

private:

  Network* network_;
  sc_signal<bool> clockSig;
  sc_core::sc_event newData;

};

#endif /* UNCLOCKEDNETWORK_H_ */
