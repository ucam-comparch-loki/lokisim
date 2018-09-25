/*
 * Multiplexer.h
 *
 *  Created on: 7 Sep 2011
 *      Author: db434
 */

#ifndef MULTIPLEXER_H_
#define MULTIPLEXER_H_

#include "../LokiComponent.h"
#include "../Utility/BlockingInterface.h"
#include "../Utility/LokiVector.h"
#include "NetworkTypes.h"

using sc_core::sc_module_name;

class Multiplexer: public LokiComponent, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

  MuxSelectInput        iSelect;

  LokiVector<DataInput> iData;
  DataOutput            oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Multiplexer);
  Multiplexer(const sc_module_name& name, int numInputs);

//============================================================================//
// Methods
//============================================================================//

public:

  int inputs() const;

protected:

  virtual void reportStalls(ostream& os);

private:

  void handleData();

//============================================================================//
// Local state
//============================================================================//

private:

  bool haveSentData;

};

#endif /* MULTIPLEXER_H_ */
