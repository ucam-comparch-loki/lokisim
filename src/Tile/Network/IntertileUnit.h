/*
 * IntertileUnit.h (Inter-tile Communication Unit in the documentation)
 *
 * Module responsible for managing core-to-core communication between tiles.
 *
 * This module handles connection set-up and tear-down, and the sending (but
 * not receiving) of credits.
 *
 *  Created on: 29 Mar 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_INTERTILEUNIT_H_
#define SRC_TILE_NETWORK_INTERTILEUNIT_H_

#include "../../LokiComponent.h"
#include "../../Network/Arbiters/RoundRobinArbiter.h"
#include "../../Network/FIFOs/NetworkFIFO.h"
#include "../../Utility/LokiVector2D.h"

class Tile;

class IntertileUnit: public LokiComponent {

  struct credit_state_t;

//============================================================================//
// Ports
//============================================================================//

public:

  // Experimenting with making the buffers directly accessible instead of
  // putting them behind ports.

  // Data received from remote cores.
  sc_port<network_sink_ifc<Word>> iData;

  // Data passed on to local cores. The connection management messages are
  // filtered out and consumed here.
  sc_port<network_source_ifc<Word>> oData;

  // Credits to remote cores.
  sc_port<network_sink_ifc<Word>> oCredit;

  // Data inputs of local cores. Used only for flow control.
  // Addressed using iFlowControl[component][buffer].
  typedef sc_port<network_sink_ifc<Word>> FlowControlInput;
  LokiVector2D<FlowControlInput> iFlowControl;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(IntertileUnit);
  IntertileUnit(sc_module_name name, const tile_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

private:

  // Method triggered whenever a new flit arrives on iData.
  void dataArrived();
  void handlePortClaim(const Flit<Word> request);
  void acceptPortClaim(credit_state_t& state, const ChannelID& source);
  void rejectPortClaim(credit_state_t& state, const ChannelID& source);

  // Send credits as frequently as possible.
  void sendCredits();
  void sendNackFlit(ChannelID destination);
  void sendCreditFlit(credit_state_t& state);

  // Convert between 1D and 2D addressing for the flow control signals.
  ChannelID indexToAddress(PortIndex index) const;
  PortIndex addressToIndex(ChannelID address) const;

  // Method triggered whenever an iFlowControl port signals that it has consumed
  // some data. This may require a credit to be sent.
  void dataConsumed(uint component, uint channel);

  // Get a reference to the Tile that contains this ICU.
  Tile& tile() const;

//============================================================================//
// Local state
//============================================================================//

private:

  // The state associated with each input buffer of each core.
  struct credit_state_t {
    IntertileUnit& parent;

    // The network address of the buffer.
    const ChannelID sinkAddress;

    // The buffer connected to this sink. This is where credits will be sent.
    ChannelID sourceAddress;

    uint creditsPending;
    bool useCredits;

    // This channel has been "unclaimed" and is waiting for its final
    // credits to be sent before being made available again.
    bool disconnectPending;

    credit_state_t(ChannelID address, IntertileUnit& parent) :
        parent(parent),
        sinkAddress(address),
        sourceAddress(ChannelID()),
        creditsPending(0),
        useCredits(false),
        disconnectPending(false) {
      // Nothing
    }

    void addCredit() {
      if (useCredits) {
        creditsPending++;
        parent.newCreditEvent.notify(sc_core::SC_ZERO_TIME);

        // Only add to the list of candidates once, with the first credit.
        if (creditsPending == 1)
          parent.creditsOutstanding.add(parent.addressToIndex(sinkAddress));
      }
    }
  };

  // One credit state per buffer being managed.
  // Addressed using state[component][buffer].
  vector<vector<credit_state_t>> creditState;

  // Source channel waiting for its connection request to be rejected.
  // Set to ChannelID() when not in use.
  ChannelID nackChannel;

  // Incoming and outgoing data. (Not credits.)
  NetworkFIFO<Word> inBuffer, outBuffer;

  // State to choose which buffer to send a credit for next.
  RoundRobinArbiter2 arbiter;
  request_list_t creditsOutstanding;

  // Event triggered whenever there is a new credit to be sent.
  // Also triggered by other events which require similar responses, e.g. a
  // failed port claim which needs a nack to be sent.
  sc_event newCreditEvent;

};

#endif /* SRC_TILE_NETWORK_INTERTILEUNIT_H_ */
