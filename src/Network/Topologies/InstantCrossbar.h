/*
 * InstantCrossbar.h
 *
 * Crossbar which does not expose its arbitration signals. Arbitration happens
 * automatically whenever data arrives.
 *
 * FIXME: while quicker than a regular crossbar, arbiters still only update
 * their state on clock edges. Consider a combination of demultiplexers and
 * WormholeMultiplexers.
 *
 *  Created on: 27 Feb 2014
 *      Author: db434
 */

#ifndef INSTANTCROSSBAR_H_
#define INSTANTCROSSBAR_H_

#include "../Network.h"
#include "Crossbar.h"

class InstantCrossbar: public Network {

public:

//============================================================================//
// Ports
//============================================================================//

// Inherited from Network:
//
//  ClockInput   clock;

  LokiVector<DataInput>  iData;
  LokiVector<DataOutput> oData;

  // A signal from each buffer of each component, telling whether it is ready
  // to receive data. Addressed using readyIn[component][buffer].
  LokiVector2D<ReadyInput>   iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component.
  // buffersPerComponent = the number of buffers each destination component has.
  //                       There will be one flow control signal per buffer.
  InstantCrossbar(const sc_module_name& name,
                  const ComponentID& ID,
                  int inputs,
                  int outputs,
                  int outputsPerComponent,
                  HierarchyLevel level,
                  int buffersPerComponent);
  virtual ~InstantCrossbar();

//============================================================================//
// Methods
//============================================================================//

protected:

  // Issue arbitration requests whenever data arrives, and send
  // acknowledgements when the data has cleared the crossbar.
  void mainLoop(const PortIndex port);

//============================================================================//
// Components
//============================================================================//

private:

  Crossbar crossbar;

  // Address using requests[input][output].
  LokiVector2D<ArbiterRequestSignal> requests;
  // Address using grants[input][output].
  LokiVector2D<ArbiterGrantSignal>   grants;

  enum CrossbarState {
    IDLE,
    ARBITRATING,
    FINISHED
  };

  // Each input port has its own state.
  std::vector<CrossbarState> state;

};

#endif /* INSTANTCROSSBAR_H_ */
