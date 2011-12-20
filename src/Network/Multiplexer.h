/*
 * Multiplexer.h
 *
 *  Created on: 7 Sep 2011
 *      Author: db434
 */

#ifndef MULTIPLEXER_H_
#define MULTIPLEXER_H_

#include "../Component.h"
#include "NetworkTypedefs.h"


class Multiplexer: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<int>   select;

  DataInput   *dataIn;
  DataOutput   dataOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Multiplexer);
  Multiplexer(const sc_module_name& name, int numInputs);
  virtual ~Multiplexer();

//==============================//
// Methods
//==============================//

public:

  int inputs() const;

private:

  void handleData();

//==============================//
// Local state
//==============================//

private:

  const int numInputs;

};

#endif /* MULTIPLEXER_H_ */
