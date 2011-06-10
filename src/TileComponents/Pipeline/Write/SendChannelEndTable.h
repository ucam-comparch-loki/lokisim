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
#include "../../../Datatype/MemoryRequest.h"
#include "../../ChannelMapEntry.h"

class AddressedWord;
class DecodedInst;
class Word;

class SendChannelEndTable: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>            clock;

  // Data output to the network.
  sc_out<AddressedWord>  output;
  sc_out<bool>           validOutput;
  sc_in<bool>            ackOutput;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  sc_in<AddressedWord>   creditsIn;
  sc_in<bool>            validCredit;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Write the result of an operation to one of the output buffers.
  bool          write(const DecodedInst& dec);

  // Write some data to a particular output.
  bool          write(const AddressedWord& data, MapIndex output);

  // Returns true if the table is incapable of accepting new data at the moment.
  bool          full() const;

  ComponentID   getSystemCallMemory() const;

private:

  // Send the oldest value in each output buffer, if the flow control signals
  // allow it.
  void          sendLoop();
  void          send();

  // Update an entry in the channel mapping table.
  void          updateMap(MapIndex entry, int64_t newVal);

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(MapIndex channel);

  // Execute a memory operation.
  bool          executeMemoryOp(MapIndex entry, MemoryRequest::MemoryOperation memoryOp, int64_t data);

  // A credit was received, so update the corresponding credit counter.
  void          receivedCredit();

//==============================//
// Local state
//==============================//

protected:

  // A buffer for outgoing data.
  BufferStorage<AddressedWord> buffer;

  // Store the map index associated with each entry in the main buffer, so we
  // know where to take credits from, etc.
  BufferStorage<MapIndex>      mapEntries;

  // Channel mapping table used to store addresses of destinations of sent
  // data. Note that mapping 0 is held both here and in the decode stage --
  // it may be possible to optimise this away at some point.
  vector<ChannelMapEntry>      channelMap;

  // Currently waiting for some event to occur. (e.g. Credits to arrive or
  // buffer to empty.)
  bool waiting;

  sc_core::sc_event dataToSendEvent;

  // Used to tell that we are not currently waiting for any output buffers
  // to empty.
  static const ChannelIndex    NO_CHANNEL = -1;

};

#endif /* SENDCHANNELENDTABLE_H_ */
