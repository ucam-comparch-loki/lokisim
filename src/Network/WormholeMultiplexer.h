/*
 * WormholeMultiplexer.h
 *
 * Multiplexer which allows whole packets at a time through. All arbitration
 * decisions are made as soon as possible - no clocking is involved.
 *
 * TODO: this is almost identical to the arbitrated multiplexer. Merge?
 *
 *  Created on: 9 Dec 2015
 *      Author: db434
 */

#ifndef SRC_NETWORK_WORMHOLEMULTIPLEXER_H_
#define SRC_NETWORK_WORMHOLEMULTIPLEXER_H_


#include "../LokiComponent.h"
#include "../Network/NetworkTypedefs.h"
#include "../Utility/Assert.h"

using sc_core::sc_module_name;

template<class T>
class WormholeMultiplexer: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

  typedef loki_in<Flit<T> > InPort;
  typedef loki_out<Flit<T> > OutPort;

public:

  // Data input.
  LokiVector<InPort> iData;

  // Data output.
  OutPort oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(WormholeMultiplexer);
  WormholeMultiplexer(const sc_module_name& name, uint inputs) :
      LokiComponent(name),
      iData(inputs, "iData"),
      oData("oData") {

    state = MUX_INIT;
    lastSelected = 0;

    SC_METHOD(mainLoop);
    for (uint i=0; i<inputs; i++)
      sensitive << iData[i];
    dont_initialize();

  }

//============================================================================//
// Methods
//============================================================================//

private:

  void mainLoop() {

    switch (state) {
      case MUX_INIT:
        selectNewInput();
        state = MUX_SELECTED;
        next_trigger(oData.ack_event());
        break;

      case MUX_READY:
        if (haveValidInput()) {
          selectInput();
          state = MUX_SELECTED;
          next_trigger(oData.ack_event());
        }
        break;

      case MUX_SELECTED:
        loki_assert(!oData.valid());

        if (iData[lastSelected].valid())
          iData[lastSelected].ack();

        if (haveValidInput()) {
          selectInput();
          next_trigger(oData.ack_event());
        }
        else {
          state = MUX_READY;
          // default trigger = any input
        }

        break;
    }

  }

  bool haveValidInput() const {
    // If we're in the middle of a packet, only check the input we selected last.
    if (!oData.read().getMetadata().endOfPacket)
      return iData[lastSelected].valid();
    else
      return haveNewInput();
  }

  bool haveNewInput() const {
    for (uint i=0; i<iData.size(); i++) {
      if (iData[i].valid())
        return true;
    }

    return false;
  }

  void selectInput() {
    loki_assert(!oData.valid());

    if (!oData.read().getMetadata().endOfPacket)
      oData.write(iData[lastSelected].read());
    else
      selectNewInput();
  }

  void selectNewInput() {
    PortIndex currentPort = lastSelected + 1;

    for (uint i=0; i<iData.size(); i++) {
      if (currentPort >= iData.size())
        currentPort = 0;

      if (iData[currentPort].valid()) {
        oData.write(iData[currentPort].read());
        lastSelected = currentPort;
        return;
      }

      currentPort++;
    }

    loki_assert_with_message(false, "Couldn't find valid input", 0);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  enum MuxState {
    MUX_INIT,     // Initialisation
    MUX_READY,    // No valid inputs
    MUX_SELECTED, // Sent output data, waiting for acknowledgement
  };

  MuxState state;

  // Use round-robin arbitration. Keep track of the previously selected input
  // so we can start checking at the next input.
  PortIndex lastSelected;

};

#endif /* SRC_NETWORK_WORMHOLEMULTIPLEXER_H_ */
