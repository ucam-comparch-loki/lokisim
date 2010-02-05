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

class FetchLogic: public Component {

/* Methods */
  void doOp();
  void haveResultFromCache();

public:
/* Ports */
  sc_in<Address> in;
  sc_in<bool> cacheContainsInst;
  sc_out<Address> toIPKC;
  sc_out<AddressedWord> toNetwork;

  // Flow control?

/* Constructors and destructors */
  SC_HAS_PROCESS(FetchLogic);
  FetchLogic(sc_core::sc_module_name name, int ID);
  virtual ~FetchLogic();
};

#endif /* FETCHLOGIC_H_ */
