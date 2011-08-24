/*
 * FetchLogic.h
 *
 * Component responsible for determining whether or not an Instruction needs
 * to be loaded from memory, and if it does, sends the request.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef FETCHLOGIC_H_
#define FETCHLOGIC_H_

#include "../../../Component.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Memory/BufferStorage.h"

class DecodeStage;

class FetchLogic: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>           clock;

  // Flow control. Can only send a request onto the network if the value
  // on this port is true.
  sc_in<bool>           flowControl;

  // The request being sent out onto the network.
  sc_out<AddressedWord> toNetwork;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchLogic);
  FetchLogic(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Send a fetch request for the instruction packet at the given address,
  // in the memory we are set to fetch from, but only if it is not already in
  // the instruction packet cache.
  // checkedCache signals whether the cache has already been checked. If it has,
  // we can issue the fetch immediately.
  void fetch(const MemoryAddr addr, bool checkedCache);

  // Update the channel to which we send fetch requests.
  void setFetchChannel(const ChannelID& channelID, uint memoryGroupBits, uint memoryLineBits);

private:

  // Prepare a request to be sent on the network.
  void sendRequest(const AddressedWord& data);

  // Send a request onto the network whenever notified by sendRequest.
  void send();

  // Find out whether the wanted instruction packet is in the cache.
  bool inCache(const MemoryAddr addr) const;

  // Find out if there is room in the cache to accommodate another instruction
  // packet. Assumes that the packet being fetched is of the maximum size.
  bool roomInCache() const;

  DecodeStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  // The network channel we send all of our fetch requests to.
  ChannelID             fetchChannel;

  // Number of group bits describing virtual memory bank.
  unsigned int          memoryGroupBits_;

  // Number of line bits describing virtual memory bank.
  unsigned int          memoryLineBits_;

  // The memory request to be sent onto the network.
  AddressedWord         dataToSend;

  // Event which is triggered whenever there is data to send.
  sc_core::sc_event     sendEvent;

};

#endif /* FETCHLOGIC_H_ */
