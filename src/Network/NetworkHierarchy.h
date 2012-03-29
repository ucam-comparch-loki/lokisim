/*
 * NetworkHierarchy.h
 *
 * A class responsible for organising all levels of hierarchy in the network
 * for both data and credits.
 *
 *  Created on: 1 Nov 2010
 *      Author: db434
 */

#ifndef NETWORKHIERARCHY_H_
#define NETWORKHIERARCHY_H_

#include <vector>
#include "../Component.h"
#include "Network.h"
#include "OffChip.h"
#include "../Datatype/AddressedWord.h"

using std::vector;

class FlowControlIn;
class FlowControlOut;

class NetworkHierarchy : public Component {

//==============================//
// Ports
//==============================//

public:

  ClockInput   clock;

  // Additional clocks which are skewed, allowing multiple clocked events
  // to happen in one cycle.
  ClockInput   fastClock, slowClock;

  // Data received from each output of each networked component.
  LokiVector<DataInput>    dataIn;

  // Data sent to each networked component (after having its address removed
  // by the flow control components).
  LokiVector<DataOutput>   dataOut;
  LokiVector<ReadyInput>   readyDataOut;

  // Flow control information received from each input of each component.
  LokiVector<CreditInput>  creditsIn;

  // A signal telling each input whether it is allowed to send a credit.
  LokiVector<CreditOutput> creditsOut;


//==============================//
// Constructors and destructors
//==============================//

public:

  NetworkHierarchy(sc_module_name name);
  virtual ~NetworkHierarchy();

//==============================//
// Methods
//==============================//

public:

  // Return a pointer to the given component's local network. This allows new
  // interfaces to be tried out more quickly.
  local_net_t* getLocalNetwork(ComponentID component) const;

private:

  void setupFlowControl();

  void makeLocalNetwork(int tileID);
  void makeGlobalNetwork();

//==============================//
// Components
//==============================//

private:

  // Flow control at each component's inputs.
  vector<FlowControlIn*> flowControlIn;

  // Flow control at each component's outputs.
  vector<FlowControlOut*> flowControlOut;

  vector<local_net_t*> localNetworks;
  global_net_t *globalDataNetwork, *globalCreditNetwork;
  OffChip offChip;  // Should this be in here, or outside?

//==============================//
// Signals (wires)
//==============================//

private:

  // Signals between the network and off-chip component.
  DataSignal    dataFromOffchip,       dataToOffchip;
  CreditSignal  creditsFromOffchip,    creditsToOffchip;
  ReadySignal   readyDataToOffchip,    readyDataFromOffchip,
                readyCreditFromOffchip;

  // Signals between local and global networks.
  LokiVector<DataSignal>   dataToLocalNet,       dataFromLocalNet;
  LokiVector<CreditSignal> creditsToLocalNet,    creditsFromLocalNet;
  LokiVector<ReadySignal>  localReadyForData,    localReadyForCredits,
                           globalReadyForData,   globalReadyForCredits;

  // Signals between off-chip and its flow control component.
  DataSignal         dataFromOffChip;
  sc_signal<Word>    dataToOffChip;
  ReadySignal        readyFromOffChip;
  ReadySignal        readyToOffChip;

};

#endif /* NETWORKHIERARCHY_H_ */
