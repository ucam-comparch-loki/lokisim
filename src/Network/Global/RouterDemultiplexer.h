/*
 * RouterDemultiplexer.h
 *
 *  A specialised demultiplexer for dealing with the signals between a router
 *  and the components on a particular tile.
 *
 *  This demultiplexer is special because it knows that each output may have
 *  multiple flow control signals behind it (e.g. one for each core buffer).
 *
 *  Created on: 9 Dec 2015
 *      Author: db434
 */

#ifndef SRC_NETWORK_GLOBAL_ROUTERDEMULTIPLEXER_H_
#define SRC_NETWORK_GLOBAL_ROUTERDEMULTIPLEXER_H_

#include "../../LokiComponent.h"
#include "../NetworkTypedefs.h"
#include "../../Utility/Assert.h"

template<class T>
class RouterDemultiplexer: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

  typedef loki_in<Flit<T> > InPort;
  typedef loki_out<Flit<T> > OutPort;

public:

  // Data input.
  InPort iData;

  // Data output.
  LokiVector<OutPort> oData;

  // A flow control signal from each buffer.
  // Addressed using iReady[component][buffer].
  LokiVector2D<ReadyInput> iReady;

  // Flow control signal to the router.
  ReadyOutput oReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(RouterDemultiplexer);
  RouterDemultiplexer(const sc_module_name& name, uint outputs, uint buffersPerOutput) :
    LokiComponent(name),
    iData("iData"),
    oData(outputs, "oData"),
    iReady(outputs, buffersPerOutput, "iReady"),
    oReady("oReady") {

    oReady.initialize(true);

    state = DEMUX_READY;

    SC_METHOD(mainLoop);
    sensitive << iData;
    dont_initialize();
  }

//============================================================================//
// Methods
//============================================================================//

private:

  void mainLoop() {

    loki_assert(iData.valid());
    ChannelID destination = iData.read().channelID();

    switch (state) {
      case DEMUX_READY:
        if (flowControlSignal(destination).read()) {
          loki_assert(!outputSignal(destination).valid());
          outputSignal(destination).write(iData.read());
          oReady.write(false);
          state = DEMUX_SENT;
          next_trigger(outputSignal(destination).ack_event());
        }
        else {
          next_trigger(flowControlSignal(destination).posedge_event());
        }
        break;

      case DEMUX_SENT:
        loki_assert(!outputSignal(destination).valid());
        iData.ack();
        oReady.write(true);
        state = DEMUX_READY;
        // default trigger = new data arriving
        break;
    }

  }

  const ReadyInput& flowControlSignal(ChannelID destination) const {
    // Cope with situations where there are multiple components/channels behind
    // a single port.
    uint component = std::min<int>(destination.component.position, iReady.size()-1);
    uint channel = std::min<int>(destination.channel, iReady[component].size()-1);

    return iReady[component][channel];
  }

  OutPort& outputSignal(ChannelID destination) const {
    uint port = std::min<int>(destination.component.position, iReady.size()-1);
    return oData[port];
  }

//============================================================================//
// Local state
//============================================================================//

private:

  enum MuxState {
    DEMUX_READY,    // Waiting to send something
    DEMUX_SENT,     // Sent output data, waiting for acknowledgement
  };

  MuxState state;

};

#endif /* SRC_NETWORK_GLOBAL_ROUTERDEMULTIPLEXER_H_ */
