/*
 * Crossbar.h
 *
 *  Created on: 8 Sep 2011
 *      Author: db434
 */

#ifndef CROSSBAR_H_
#define CROSSBAR_H_

#include "../Network.h"
#include "../../Utility/BlockingInterface.h"
#include "../../Utility/LokiVector2D.h"

class ClockedArbiter;
class Multiplexer;

class Crossbar: public Network, public BlockingInterface {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput   clock;

  LokiVector<DataInput>  iData;
  LokiVector<DataOutput> oData;

  // A request/grant signal for each input to reserve each output.
  // Indexed as: iRequest[input][output]
  LokiVector2D<ArbiterRequestInput> iRequest;
  LokiVector2D<ArbiterGrantOutput>  oGrant;

  // A signal from each buffer of each component, telling whether it is ready
  // to receive data. Addressed using iReady[component][buffer].
  LokiVector2D<ReadyInput>   iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component.
  // buffersPerComponent = the number of buffers each destination component has.
  //                       There will be one flow control signal per buffer.
  Crossbar(const sc_module_name& name,
           const ComponentID& ID,
           int inputs,
           int outputs,
           int outputsPerComponent,
           HierarchyLevel level,
           int buffersPerComponent);
  virtual ~Crossbar();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void inputChanged(const PortIndex port);
  virtual void outputChanged(const PortIndex port);

  virtual void reportStalls(ostream& os);

private:

  void makePorts(uint inputs, uint outputs);
  void makeSignals();
  void makeArbiters();
  void makeMuxes();

//============================================================================//
// Components
//============================================================================//

protected:

  const int numArbiters, numMuxes;

  // The number of output ports which lead to the same component. Each
  // component will have one arbiter, but this many multiplexers.
  const int outputsPerComponent;

  // We will need a flow control signal from each buffer of each component.
  // This value tells how many there are.
  const int buffersPerComponent;

  std::vector<ClockedArbiter*> arbiters;
  std::vector<Multiplexer*>  muxes;

  LokiVector2D<MuxSelectSignal> selectSig;

  // Store the old data so we can compute switching activity.
  // TODO: do this in the wire modules, where they already store this info.
  //   Issue: wires don't know what sort of network they are a part of, if any.
  std::vector<NetworkData> oldInputs, oldOutputs;

};

#endif /* CROSSBAR_H_ */
