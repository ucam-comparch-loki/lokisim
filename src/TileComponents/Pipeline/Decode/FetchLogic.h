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
#include "../../../Datatype/Address.h"
#include "../../../Datatype/AddressedWord.h"
#include "../../../Datatype/Data.h"
#include "../../../Memory/Buffer.h"

class FetchLogic: public Component {

//==============================//
// Ports
//==============================//

public:

  // The offset from the base address that we want the instruction packet from.
  sc_in<Address>        in;

  // The base address of the instruction packet we want.
  sc_in<Data>           baseAddress;

  // Status flags from the instruction packet cache.
  sc_in<bool>           cacheContainsInst, isRoomToFetch;

  // Flow control. Can only send a request onto the network if the value
  // on this port is true.
  sc_in<bool>           flowControl;

  // Signal to the cluster that it should stall until the network request has
  // been sent. Prevents the cluster attempting further fetches which may
  // result in the loss of data.
  sc_out<bool>          stallOut;

  // The address to lookup in the instruction packet cache.
  sc_out<Address>       toIPKC;

  // The request being sent out onto the network.
  sc_out<AddressedWord> toNetwork;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(FetchLogic);
  FetchLogic(sc_module_name name, int ID);
  virtual ~FetchLogic();

//==============================//
// Methods
//==============================//

private:

  // When a request for an instruction is received, see if it is in the cache.
  void doOp();

  // If the cache doesn't contain the desired instruction, send a request to
  // the memory. Otherwise, we don't have to do anything.
  void haveResultFromCache();

  // Add the base address onto our offset address, but only if we're expecting
  // it. We are expecting it if we have just received a fetch request.
  void haveBaseAddress();

  // The flowControl signal tells us that we are now free to use the channel
  // again, so send the next request, if there is one.
  void sendNext();

//==============================//
// Local state
//==============================//

private:

  // A queue of requests, waiting to be sent.
  Buffer<AddressedWord> toSend;

  // Tells us whether we are expecting to receive data from the register file.
  bool                  awaitingBaseAddr;

  // The address to be combined with the base address to form the address we
  // want to access.
  Address               offsetAddr;

//==============================//
// Signals (wires)
//==============================//

private:

  sc_signal<bool>       sendData;

};

#endif /* FETCHLOGIC_H_ */
