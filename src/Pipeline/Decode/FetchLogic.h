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

#include "../../Component.h"
#include "../../Datatype/Address.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Data.h"
#include "../../Memory/Buffer.h"

class FetchLogic: public Component {

/* Local state */
  Buffer<AddressedWord> toSend;
  bool canSend;
  Address offsetAddr;

/* Signals (wires) */
  sc_signal<bool> sendData;

/* Methods */
  void doOp();
  void haveResultFromCache();
  void haveBaseAddress();
  void sendNext();

public:
/* Ports */
  sc_in<Address> in;
  sc_in<Data> baseAddress;
  sc_in<bool> cacheContainsInst, isRoomToFetch, flowControl;
  sc_out<Address> toIPKC;
  sc_out<AddressedWord> toNetwork;

/* Constructors and destructors */
  SC_HAS_PROCESS(FetchLogic);
  FetchLogic(sc_core::sc_module_name name, int ID);
  virtual ~FetchLogic();
};

#endif /* FETCHLOGIC_H_ */
