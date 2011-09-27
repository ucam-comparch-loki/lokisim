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

class AddressedWord;
class ChannelMapTable;
class DecodedInst;
class Word;
class WriteStage;

// Into the output buffers, we store both the data to send, and its associated
// channel map table entry. This increases the buffer width by 4 bits, but
// allows the pipeline to continue for longer, as credits can be checked as
// late as possible.
typedef std::pair<AddressedWord, MapIndex> BufferedInfo;

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
  SendChannelEndTable(sc_module_name name, const ComponentID& ID, ChannelMapTable* cmt);
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

  // A handle for an event which triggers whenever the send channel-end table
  // might stall or unstall.
  const sc_event& stallChangedEvent() const;

private:

  // Send the oldest value in each output buffer, if the flow control signals
  // allow it.
  void          sendLoop2();
  void          sendLoop(unsigned int buffer);
  void          send(unsigned int buffer);

  // A separate thread/method for each buffer, making them completely independent.
  void          sendToCores();
  void          sendToMemories();
  void          sendOffTile();

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(MapIndex channel);

  // A credit was received, so update the corresponding credit counter.
  void          receivedCredit(unsigned int buffer);

  void          creditFromCores();
  void          creditFromMemories(); // For consistency only. Should be unused.
  void          creditFromOffTile();

  WriteStage*   parent() const;

//==============================//
// Local state
//==============================//

private:

  // Use a different buffer depending on the destination to avoid deadlock,
  // and allow different flow control, etc.
  enum BufferIndex {TO_CORES = 0, TO_MEMORIES = 1, OFF_TILE = 2};

  enum SendState {
    IDLE,
    HAVE_DATA,
    SENT_DATA
  };

  SendState state;

  // Buffers for storing outgoing data. One buffer for each network.
  BufferArray<BufferedInfo> buffers;

  // A pointer to this core's channel map table. The table itself is in the
  // Cluster class. No reading or writing of destinations should occur here -
  // this part of the core should only deal with credits.
  ChannelMapTable* channelMapTable;

  // Currently waiting for some event to occur. (e.g. Credits to arrive or
  // buffer to empty.)
  bool waiting;

  // An event for each buffer, which is triggered whenever data is inserted
  // into the buffer.
  sc_event* dataToSendEvent;   // array
  sc_event  bufferFillChanged;

  // Used to tell that we are not currently waiting for any output buffers
  // to empty.
  static const ChannelIndex  NO_CHANNEL = -1;

};

#endif /* SENDCHANNELENDTABLE_H_ */
