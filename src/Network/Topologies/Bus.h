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
  ReadyInput    validDataIn;
  ReadyOutput   ackDataIn;

  DataOutput   *dataOut;
  ReadyOutput  *validDataOut;
  ReadyInput   *ackDataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Bus);

  // channelsPerOutput = number of channel IDs accessible through each output
  //                     of this bus. e.g. A memory may have 8 input channels,
  //                     which share only one input port.
  // startAddr         = the first channelID accessible through this network.
  // TODO: remove numOutputChannels and startAddr when ChannelIDs are updated
  //       to hold (tile, component, channel).
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
