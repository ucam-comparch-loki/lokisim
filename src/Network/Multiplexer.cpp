/*
 * Multiplexer.cpp
 *
 *  Created on: 7 Sep 2011
 *      Author: db434
 */

#include "Multiplexer.h"
#include "../Utility/Assert.h"

void Multiplexer::reportStalls(ostream& os) {
  if (oData.valid()) {
    os << this->name() << " hasn't received ack for " << oData.read() << endl;
  }
}

void Multiplexer::handleData() {
  switch (state) {
    case MUX_IDLE: {
      selectedInput = iSelect.read();

      if (selectedInput == NO_SELECTION)
        next_trigger(iSelect.default_event());
      else {
        // Assuming that we will definitely send data from the selected input.
        state = MUX_SEND;
        loki_assert((PortIndex)selectedInput < iData.size());

        if (iData[selectedInput].valid())
          next_trigger(sc_core::SC_ZERO_TIME);
        else
          next_trigger(iData[selectedInput].default_event());
      }

      break;
    }

    case MUX_SEND: {
      loki_assert(selectedInput != NO_SELECTION);
      loki_assert(iData[selectedInput].valid());

      oData.write(iData[selectedInput].read());
//      cout << this->name() << " sent " << iData[selectedInput].read() << endl;

      state = MUX_ACKNOWLEDGE;
      next_trigger(oData.ack_event());
      break;
    }

    case MUX_ACKNOWLEDGE: {
      loki_assert(selectedInput != NO_SELECTION);

      iData[selectedInput].ack();
//      cout << this->name() << " acknowledged " << iData[selectedInput].read() << endl;

      state = MUX_IDLE;
      next_trigger(iSelect.default_event() | iData[selectedInput].default_event());
      break;
    }
  }
}

Multiplexer::Multiplexer(const sc_module_name& name, int numInputs) :
    LokiComponent(name),
    iSelect("iSelect"),
    iData("iData", numInputs),
    oData("oData") {

  loki_assert(numInputs > 0);

  state = MUX_IDLE;
  selectedInput = NO_SELECTION;

  SC_METHOD(handleData);

}
