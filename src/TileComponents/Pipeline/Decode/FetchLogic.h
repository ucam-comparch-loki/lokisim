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

  // Flow control. Can only send a request onto the network if the value
  // on this port is true.
  sc_in<bool>           flowControl;

  // The request being sent out onto the network.
  sc_out<AddressedWord> toNetwork;

//==============================//
// Constructors and destructors
//==============================//

public:

  FetchLogic(sc_module_name name, int ID);
  virtual ~FetchLogic();

//==============================//
// Methods
//==============================//

public:

  // Send a fetch request for the instruction packet at the given address,
  // in the memory we are set to fetch from, but only if it is not already in
  // the instruction packet cache.
  void fetch(const MemoryAddr addr);

  // Update the channel to which we send fetch requests.
  void setFetchChannel(const ChannelID channelID);

  // The packet to fetch was in the cache at fetch time, but has since been
  // overwritten. Send a fetch request to get it back.
  void refetch();

  // Attempt to send the fetch request onto the network. This may fail if
  // flow control stops it, or if there is not enough room in the cache.
  void send();

private:

  // Find out whether the wanted instruction packet is in the cache.
  bool inCache(const MemoryAddr addr) const;

  // Find out if there is room in the cache to accommodate another instruction
  // packet. Assumes that the packet being fetched is of the maximum size.
  bool roomInCache() const;

  // Returns a unique identifier for this fetch logic's output port.
  ChannelID portID() const;

  DecodeStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  // A queue of requests, waiting to be sent.
  BufferStorage<AddressedWord> toSend;

  // The network channel we send all of our fetch requests to.
  ChannelID             fetchChannel;

  // A memory request for the next packet to be executed, where the packet is
  // already in the cache. We need to be able to refetch the packet in case
  // it gets overwritten.
  AddressedWord         refetchRequest;

};

#endif /* FETCHLOGIC_H_ */
