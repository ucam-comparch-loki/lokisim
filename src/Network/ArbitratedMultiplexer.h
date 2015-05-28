/*
 * ArbitratedMultiplexer.h
 *
 * Multiplexer which chooses between its inputs in a fair manner.
 *
 * A new input is selected whenever the output of the multiplexer is
 * acknowledged. The currently selected input is also acknowledged at this time.
 *
 *  Created on: 27 Nov 2014
 *      Author: db434
 */

#ifndef ARBITRATEDMULTIPLEXER_H_
#define ARBITRATEDMULTIPLEXER_H_

#include "../Component.h"
#include "../Network/NetworkTypedefs.h"

template<class T>
class ArbitratedMultiplexer: public Component {

//==============================//
// Ports
//==============================//

  typedef loki_in<T> InPort;
  typedef loki_out<T> OutPort;

public:

  // Data input.
  LokiVector<InPort> iData;

  // Data output.
  OutPort oData;

  // Only provide data from the currently-selected input until this signal is
  // deasserted.
  sc_in<bool> iHold;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ArbitratedMultiplexer);
  ArbitratedMultiplexer(const sc_module_name& name, uint inputs) : Component(name) {
    iData.init(inputs);

    state = MUX_READY;
    lastSelected = 0;

    SC_METHOD(mainLoop);
    for (uint i=0; i<inputs; i++)
      sensitive << iData[i];
    //sensitive << iHold.neg(); // Look at other inputs when hold is released
    dont_initialize();
  }

//==============================//
// Methods
//==============================//

private:

  void mainLoop() {

    switch (state) {
      case MUX_READY:
        if (haveValidInput()) {
          selectInput();
          state = MUX_SELECTED;
          next_trigger(oData.ack_event());
        }
        break;

      case MUX_SELECTED:
        assert(!oData.valid());

        if (iData[lastSelected].valid())
          iData[lastSelected].ack();

        if (haveValidInput()) {
          selectInput();
          next_trigger(oData.ack_event());
        }
        else
          state = MUX_READY;
          // default trigger = any input

        break;
    }

  }

  bool haveValidInput() const {
    // If the input is being held, only check the input we selected last.
    if (iHold.read())
      return iData[lastSelected].valid();
    else {
      for (uint i=0; i<iData.length(); i++) {
        if (iData[i].valid())
          return true;
      }

      return false;
    }
  }

  void selectInput() {
    assert(!oData.valid());

    if (iHold.read()) {
      oData.write(iData[lastSelected].read());
      return;
    }
    else {
      PortIndex currentPort = lastSelected + 1;

      for (uint i=0; i<iData.length(); i++) {
        if (currentPort >= iData.length())
          currentPort = 0;

        if (iData[currentPort].valid()) {
          oData.write(iData[currentPort].read());
          lastSelected = currentPort;
          return;
        }

        currentPort++;
      }

      assert(false && "Couldn't find valid input");
    }
  }

//==============================//
// Local state
//==============================//

private:

  enum MuxState {
    MUX_READY,    // No valid inputs
    MUX_SELECTED, // Sent output data, waiting for acknowledgement
  };

  MuxState state;

  // Use round-robin arbitration. Keep track of the previously selected input
  // so we can start checking at the next input.
  PortIndex lastSelected;

};

#endif /* ARBITRATEDMULTIPLEXER_H_ */
