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

#include "../../../LokiComponent.h"
#include "../../../Network/NetworkBuffer.h"
#include "../../../Network/NetworkTypedefs.h"
#include "../../../Utility/BlockingInterface.h"

class ChannelMapTable;
class DecodedInst;
class Word;
class WriteStage;

class SendChannelEndTable: public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput              clock;

  // Data inputs from the pipeline. Fetch comes from the Fetch stage, and Data
  // comes from the Execute stage. The Fetch has priority, unless we are part
  // way through a two-stage store operation.
  DataInput               iFetch;
  DataInput               iData;

  // Data outputs to the network.
  DataOutput              oDataLocal;
  DataOutput              oDataGlobal;

  // Credits received over the network. Each credit will still have its
  // destination attached, so we know which table entry to give the credit to.
  CreditInput             iCredit;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(SendChannelEndTable);
  SendChannelEndTable(sc_module_name name, const ComponentID& ID, ChannelMapTable* cmt);

//============================================================================//
// Methods
//============================================================================//

public:

  // Returns true if the table is incapable of accepting new data at the moment.
  bool          full() const;

  // A handle for an event which triggers whenever the send channel-end table
  // might stall or unstall.
  const sc_event& stallChangedEvent() const;

  // Process a new credit.
  void          receiveCreditInternal(const NetworkCredit& credit);

protected:

  virtual void  reportStalls(ostream& os);

private:

  // Move data from the two input ports to the output buffer(s), giving
  // priority where appropriate.
  void          receiveLoop();

  // Write some data to the output buffer.
  void          write(const NetworkData data);

  // Send the oldest value in the output buffer, if the flow control signals
  // allow it.
  void          sendLoopLocal();
  void          sendLoopGlobal();

  // A credit was received, so update the corresponding credit counter.
  void          receivedCredit();

  // Send a request to reserve (or release) a connection to a particular
  // destination component. May cause re-execution of the calling method when
  // the request is granted.
  void          requestArbitration(ChannelID destination, bool request);
  bool          requestGranted(ChannelID destination) const;

  WriteStage*   parent() const;

//============================================================================//
// Local state
//============================================================================//

private:

  enum ReceiveState {
    RS_READY,     // Ready to accept new data
    RS_PACKET     // Avoid separating multiple flits from a single packet
  };

  ReceiveState receiveState;

  enum SendState {
    SS_IDLE,
    SS_DATA_READY,
    SS_ARBITRATING,
    SS_CAN_SEND
  };

  SendState sendState;

  // Buffer of data to send onto the network.
  NetworkBuffer<NetworkData> bufferLocal, bufferGlobal;

  // A pointer to this core's channel map table. The table itself is in the
  // Core class. No reading or writing of destinations should occur here -
  // this part of the core should only deal with credits.
  ChannelMapTable* const channelMapTable;

  // An event which is triggered whenever data is inserted into the buffer.
  sc_event  bufferFillChanged;

};

#endif /* SENDCHANNELENDTABLE_H_ */