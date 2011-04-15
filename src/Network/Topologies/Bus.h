/*
 * Bus.h
 *
 *  Created on: 31 Mar 2011
 *      Author: db434
 */

#ifndef BUS_H_
#define BUS_H_

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"

typedef AddressedWord       DataType;
typedef AddressedWord       CreditType;
typedef bool                ReadyType;

typedef sc_in<DataType>     DataInput;
typedef sc_out<DataType>    DataOutput;
typedef sc_in<ReadyType>    ReadyInput;
typedef sc_out<ReadyType>   ReadyOutput;
typedef sc_in<CreditType>   CreditInput;
typedef sc_out<CreditType>  CreditOutput;

// (width, height) of this network, used to determine switching activity.
typedef std::pair<double,double> Dimension;

class Bus: public Component {

//==============================//
// Ports
//==============================//

public:

  DataInput     dataIn;
  DataOutput   *dataOut;

  // Set to 1 when sending new data, and clear when recipient has consumed it.
  ReadyOutput  *validOut;

  // Input which toggles whenever the recipient has consumed the data sent to
  // it. This means it is safe to send the next data.
  ReadyInput   *ackIn;

  ReadyOutput   canReceiveData;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Bus);

  // channelsPerOutput = number of channel IDs accessible through each output
  //                     of this bus. e.g. A memory may have 8 input channels,
  //                     which share only one input port.
  // startAddr         = the first channelID accessible through this network.
  Bus(sc_module_name name, ComponentID ID, int numOutputPorts,
      int numOutputChannels, ChannelID startAddr, Dimension size);
  virtual ~Bus();

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

  // Compute which output of this bus will be used by the given address.
  PortIndex getDestination(ChannelID address) const;

//==============================//
// Local state
//==============================//

protected:

  int numOutputs;

  // The lowest ChannelID accessible through this bus.
  ChannelID startAddress;

  // The number of ChannelIDs accessible through each output of this bus.
  int channelsPerOutput;

private:

  // The physical size of this network.
  Dimension size;

  // Store the previous value, so we can compute how many bits change when a
  // new value arrives.
  DataType lastData;

};

#endif /* BUS_H_ */
