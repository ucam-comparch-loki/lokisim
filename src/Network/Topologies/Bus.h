/*
 * Bus.h
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#ifndef BUS_H_
#define BUS_H_

#include "../../Component.h"
#include <list>

class AddressedWord;

typedef AddressedWord       DataType;
typedef AddressedWord       CreditType;
typedef sc_in<DataType>     DataInput;
typedef sc_out<DataType>    DataOutput;
typedef sc_in<bool>         ReadyInput;
typedef sc_out<bool>        ReadyOutput;
typedef sc_in<CreditType>   CreditInput;
typedef sc_out<CreditType>  CreditOutput;

class Bus: public Component {

//==============================//
// Ports
//==============================//

public:

  DataInput     dataIn;
  DataOutput   *dataOut;

  // Set to 1 when sending new data, and receiver sets to 0 when they have
  // consumed it.
  sc_inout<bool> *newData;

  ReadyOutput   readyOut;


//==============================//
// Constructors and destructors
//==============================//

public:

  // channelsPerOutput = number of channel IDs accessible through each output
  //                     of this bus. e.g. A memory may have 8 input channels,
  //                     which share only one input port.
  // startAddr         = the first channelID accessible through this network.
  Bus(sc_module_name name, ComponentID ID, int numOutputs,
      int channelsPerOutput, ChannelID startAddr);
  virtual ~Bus();

//==============================//
// Methods
//==============================//

private:

  void receivedData();
  void receivedCredit();

  // Compute which outputs of this bus will be used by the given address. This
  // allows an address to represent multiple destinations.
  void getDestinations(ChannelID address, std::vector<uint8_t>& outputs) const;

//==============================//
// Local state
//==============================//

private:

  int numOutputs;

  // The lowest ChannelID accessible through this bus.
  ChannelID startAddress;

  // The number of ChannelIDs accessible through each output of this bus.
  int channelsPerOutput;

  // Multicast is complicated unless we keep track of which outputs owe credits.
  std::list<uint8_t> outstandingCredits;

};

#endif /* BUS_H_ */
