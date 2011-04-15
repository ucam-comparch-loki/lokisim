/*
 * Network.h
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "../Component.h"

class AddressedWord;

typedef AddressedWord       DataType;
typedef AddressedWord       CreditType;
typedef bool                ReadyType;

typedef sc_buffer<CreditType> CreditSignal;
typedef sc_buffer<DataType>   DataSignal;
typedef sc_signal<ReadyType>  ReadySignal;

typedef sc_in<DataType>     DataInput;
typedef sc_out<DataType>    DataOutput;
typedef sc_in<ReadyType>    ReadyInput;
typedef sc_out<ReadyType>   ReadyOutput;
typedef sc_in<CreditType>   CreditInput;
typedef sc_out<CreditType>  CreditOutput;

// (width, height) of this network, used to determine switching activity.
typedef std::pair<double,double> Dimension;

class Network : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  DataInput    *dataIn;
  ReadyInput   *validDataIn;
  ReadyOutput  *ackDataIn;

  DataOutput   *dataOut;
  ReadyOutput  *validDataOut;
  ReadyInput   *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  Network(sc_module_name name,
          ComponentID ID,
          int numInputs,        // Number of inputs this network has
          int numOutputs,       // Number of outputs this network has
          Dimension size,       // The physical size of this network (width, height)
          bool externalConnection=false); // Is there a port to send data on if it
                                          // isn't for any local component?

  virtual ~Network();

//==============================//
// Methods
//==============================//

public:

  // The input port to this network which comes from the next level of network
  // hierarchy (or off-chip).
  DataInput& externalInput() const;

  // The output port of this network which goes to the next level of network
  // hierarchy (or off-chip).
  DataOutput& externalOutput() const;

  ReadyInput& externalValidInput() const;
  ReadyOutput& externalValidOutput() const;

  ReadyInput& externalReadyInput() const;
  ReadyOutput& externalReadyOutput() const;

//==============================//
// Local state
//==============================//

protected:

  int numInputs, numOutputs;

  // Tells whether this network has an extra connection to handle data which
  // isn't for any local component. This extra connection will typically
  // connect to the next level of the network hierarchy.
  bool externalConnection;

  Dimension size;

};

#endif /* NETWORK_H_ */
