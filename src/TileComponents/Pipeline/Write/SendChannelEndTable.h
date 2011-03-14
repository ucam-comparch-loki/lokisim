/*
 * SendChannelEndTable.h
 *
 * Component responsible for sending data out onto the network.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef SENDCHANNELENDTABLE_H_
#define SENDCHANNELENDTABLE_H_

#include "../../../Component.h"
#include "../../../Memory/BufferStorage.h"
#include "../../ChannelMapEntry.h"

class AddressedWord;
class DecodedInst;
class Word;

class SendChannelEndTable: public Component {

//==============================//
// Ports
//==============================//

public:

  // Data output to the network.
  sc_out<AddressedWord>  output;

  // A select signal to tell which network this data should be sent on.
  sc_out<int>            network;

  // A flow control signal saying whether or not the network is ready for more
  // data.
  sc_in<bool>            flowControl;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  sc_in<AddressedWord>   creditsIn;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name, ComponentID ID);

//==============================//
// Methods
//==============================//

public:

  // Write the result of an operation to one of the output buffers.
  void          write(const DecodedInst& dec);

  // Write some data to a particular output.
  void          write(const AddressedWord& data, MapIndex output);

  // Returns true if the table is incapable of accepting new data at the moment.
  bool          full() const;

  // Send the oldest value in each output buffer, if the flow control signals
  // allow it.
  void          send();

private:

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(MapIndex channel);

  // Update an entry in the channel mapping table.
  void          updateMap(MapIndex entry, ChannelID newVal);

  // Retrieve an entry from the channel mapping table.
  ChannelID     getChannel(MapIndex mapEntry) const;

  // Compute the global channel ID of the given output channel. Note that
  // since this is an output channel, the ID computation is different to
  // input channels.
  ChannelID     portID(ChannelIndex channel) const;

  // Generate a memory request using the address from the ALU and the operation
  // supplied by the decoder. The memory request will be sent to a memory and
  // will result in an operation being carried out there.
  Word          makeMemoryRequest(const DecodedInst& dec) const;

  // A credit was received, so update the corresponding credit counter.
  void          receivedCredit();

//==============================//
// Local state
//==============================//

protected:

  // A buffer for outgoing data.
  BufferStorage<AddressedWord> buffer;

  // Channel mapping table used to store addresses of destinations of sent
  // data. Note that mapping 0 is held both here and in the decode stage --
  // it may be possible to optimise this away at some point.
  vector<ChannelMapEntry>      channelMap;

  // Anything sent to the null channel ID doesn't get sent at all. This allows
  // more code re-use, even in situations where we don't want to send results.
  static const ChannelID       NULL_MAPPING = -1;

  // Used to tell that we are not currently waiting for any output buffers
  // to empty.
  static const ChannelIndex    NO_CHANNEL = -1;

};

#endif /* SENDCHANNELENDTABLE_H_ */
