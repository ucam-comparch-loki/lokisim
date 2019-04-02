/*
 * ReceiveChannelEndTable.h
 *
 * Component containing a buffer storing data from each input channel.
 *
 * Reads from the table are blocking: if a buffer being read from is empty,
 * the cluster will stall until data has arrived there.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef RECEIVECHANNELENDTABLE_H_
#define RECEIVECHANNELENDTABLE_H_

#include "../../../LokiComponent.h"
#include "../../../Network/FIFOs/NetworkFIFO.h"
#include "../../../Network/NetworkTypes.h"
#include "../../../Utility/BlockingInterface.h"
#include "../../../Utility/LokiVector.h"
#include "../../../Utility/LoopCounter.h"

class DecodeStage;
class Word;

class ReceiveChannelEndTable: public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput   clock;

  // Data values received over the network. There should be NUM_RECEIVE_CHANNELS
  // inputs in the array.
  typedef sc_port<network_sink_ifc<Word>> InPort;
  LokiVector<InPort> iData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ReceiveChannelEndTable);
  ReceiveChannelEndTable(const sc_module_name& name, size_t numChannels,
                         const fifo_parameters_t& fifoParams);

//============================================================================//
// Methods
//============================================================================//

public:

  // Read from the specified channel end. ChannelIndex 0 is mapped to r2.
  int32_t read(ChannelIndex channelEnd);

  // Non-blocking read operation which doesn't consume any data in the buffer.
  // ChannelIndex 0 is mapped to r2.
  int32_t readInternal(ChannelIndex channelEnd) const;

  // Put data in a buffer without touching the network.
  void writeInternal(ChannelIndex channel, int32_t data);

  // Return whether or not the channel contains data. ChannelIndex 0 is mapped
  // to r2.
  bool testChannelEnd(ChannelIndex channelEnd) const;

  // Return the index of a channel which currently contains data, or throw
  // an exception if none do. The index returned is the channel's register-
  // mapped index (e.g. if channel 0 contains data, 2 would be returned).
  // The bitmask selects a subset of channels that we are interested in.
  // The least significant bit represents channel 0 (r2).
  ChannelIndex selectChannelEnd(unsigned int bitmask, const DecodedInst& inst);

  // Event which is triggered whenever data arrives on a particular channel.
  // ChannelIndex 0 is mapped to r2.
  const sc_event& receivedDataEvent(ChannelIndex buffer) const;

protected:

  virtual void reportStalls(ostream& os);

private:

  // Wait for data to arrive on one of the channels specified in the bitmask.
  // The least significant bit represents channel 0 (r2).
  void waitForData(unsigned int bitmask, const DecodedInst& inst);

  // Method triggered whenever new data arrives.
  void networkDataArrived(ChannelIndex buffer);

  ComponentID  id() const;
  DecodeStage& parent() const;

//============================================================================//
// Local state
//============================================================================//

public:

  // The operations the receive channel-end table can carry out.
  enum ChannelOp {TSTCH, SELCH};

private:

  // A buffer for each channel-end.
  LokiVector<NetworkFIFO<Word>> buffers;

  // Allows round-robin selection of channels when executing selch.
  LoopCounter       currentChannel;

  // One event for the whole table to signal when new data has arrived.
  sc_event newData;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
