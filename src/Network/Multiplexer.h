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

protected:

  virtual void reportStalls(ostream& os);

private:

  void handleData();

//============================================================================//
// Local state
//============================================================================//

private:

  enum MuxState {
    MUX_IDLE,         // Wait for both a select signal and data on that input
    MUX_SEND,         // Forward the selected data to the output
    MUX_ACKNOWLEDGE   // Acknowledge the input
  };

  MuxState state;

  // Record the previous selected input. The select signal may change, so is
  // unreliable in some situations.
  MuxSelect selectedInput;

};

#endif /* MULTIPLEXER_H_ */
