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
#include "../../../Utility/Blocking.h"
#include "../../../Network/NetworkBuffer.h"
#include "../../../Network/NetworkTypedefs.h"

class ChannelMapTable;
class DecodedInst;
class Word;
class WriteStage;

class SendChannelEndTable: public Component, public Blocking {

//==============================//
// Ports
//==============================//

public:

  ClockInput              clock;

  // Data outputs to the network.
  DataOutput              oDataLocal;
  DataOutput              oDataGlobal;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  CreditInput             iCredit;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name, const ComponentID& ID, ChannelMapTable* cmt);

//==============================//
// Methods
//==============================//

public:

  // Write some data to the output buffer.
  void          write(const DecodedInst& data);

  // Returns true if the table is incapable of accepting new data at the moment.
  bool          full() const;

  // A handle for an event which triggers whenever the send channel-end table
  // might stall or unstall.
  const sc_event& stallChangedEvent() const;

protected:

  virtual void  reportStalls(ostream& os);

private:

  // Send the oldest value in the output buffer, if the flow control signals
  // allow it.
  void          sendLoopLocal();
  void          sendLoopGlobal();

  // Stall the pipeline until the channel specified is empty.
  void          waitUntilEmpty(MapIndex channel);

  // A credit was received, so update the corresponding credit counter.
  void          receivedCredit();

  // Send a request to reserve (or release) a connection to a particular
  // destination component. May cause re-execution of the calling method when
  // the request is granted.
  void          requestArbitration(ChannelID destination, bool request);
  bool          requestGranted(ChannelID destination) const;

  WriteStage*   parent() const;

//==============================//
// Local state
//==============================//

private:

  enum SendState {
    IDLE,
    DATA_READY,
    ARBITRATING,
    CAN_SEND
  };

  SendState state;

  // Buffer of data to send onto the network.
  NetworkBuffer<DecodedInst> bufferLocal, bufferGlobal;

  // A pointer to this core's channel map table. The table itself is in the
  // Core class. No reading or writing of destinations should occur here -
  // this part of the core should only deal with credits.
  ChannelMapTable* const channelMapTable;

  // Currently waiting for some event to occur. (e.g. Credits to arrive or
  // buffer to empty.)
  bool waiting;

  // An event which is triggered whenever data is inserted into the buffer.
  sc_event  bufferFillChanged;

};

#endif /* SENDCHANNELENDTABLE_H_ */
