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
#include "../../../Memory/BufferArray.h"
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

  // Data outputs to the network.
  sc_out<AddressedWord> *output;
  sc_out<bool>          *validOutput;
  sc_in<bool>           *ackOutput;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  sc_in<AddressedWord>  *creditsIn;
  sc_in<bool>           *validCredit;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name, const ComponentID& ID);
  virtual ~SendChannelEndTable();

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

  // A handle for an event which triggers whenever the send channel-end table
  // might stall or unstall.
  const sc_core::sc_event& stallChangedEvent() const;

private:

  // Send the oldest value in each output buffer, if the flow control signals
  // allow it.
  void          sendLoop(unsigned int buffer);
  void          send(unsigned int buffer);

  // A separate thread/method for each buffer, making them completely independent.
  void          sendToCores();
  void          sendToMemories();
  void          sendOffTile();

  // Update an entry in the channel mapping table.
  void          updateMap(MapIndex entry, int64_t newVal);

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(MapIndex channel);

  // Execute a memory operation.
  bool          executeMemoryOp(MapIndex entry, MemoryRequest::MemoryOperation memoryOp, int64_t data);

  // A credit was received, so update the corresponding credit counter.
  void          receivedCredit(unsigned int buffer);

  void          creditFromCores();
  void          creditFromMemories(); // For consistency only. Should be unused.
  void          creditFromOffTile();

//==============================//
// Local state
//==============================//

private:

  // Use a different buffer depending on the destination to avoid deadlock,
  // and allow different flow control, etc.
  static const unsigned int TO_CORES = 0;
  static const unsigned int TO_MEMORIES = 1;
  static const unsigned int OFF_TILE = 2;

  static const unsigned int NUM_BUFFERS = 3;

  // A buffer for outgoing data.
  BufferArray<AddressedWord> buffers;

  // Store the map index associated with each entry in the main buffer, so we
  // know where to take credits from, etc.
  BufferArray<MapIndex>      mapEntries;

  // Channel mapping table used to store addresses of destinations of sent
  // data. Note that mapping 0 is held both here and in the decode stage --
  // it may be possible to optimise this away at some point.
  vector<ChannelMapEntry>    channelMap;

  // Currently waiting for some event to occur. (e.g. Credits to arrive or
  // buffer to empty.)
  bool waiting;

  // An event for each buffer, which is triggered whenever data is inserted
  // into the buffer.
  sc_core::sc_event* dataToSendEvent;   // array
  sc_core::sc_event  bufferFillChanged;

  // Used to tell that we are not currently waiting for any output buffers
  // to empty.
  static const ChannelIndex  NO_CHANNEL = -1;

};

#endif /* SENDCHANNELENDTABLE_H_ */
