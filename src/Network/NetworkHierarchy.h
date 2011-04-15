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

  sc_in<bool>   clock;

  // Data received from each output of each networked component.
  DataInput    *dataIn;
  ReadyInput   *validDataIn;
  ReadyOutput  *ackDataIn;

  // Data sent to each networked component (after having its address removed
  // by the flow control components).
  DataOutput   *dataOut;
  ReadyOutput  *validDataOut;
  ReadyInput   *ackDataOut;

  // Flow control information received from each input of each component.
  CreditInput  *creditsIn;
  ReadyInput   *validCreditIn;
  ReadyOutput  *ackCreditIn;

  // A signal telling each input whether it is allowed to send a credit.
  CreditOutput *creditsOut;
  ReadyOutput  *validCreditOut;
  ReadyInput   *ackCreditOut;


//==============================//
// Constructors and destructors
//==============================//

public:

  NetworkHierarchy(sc_module_name name);
  virtual ~NetworkHierarchy();

//==============================//
// Methods
//==============================//

private:

  void setupFlowControl();

  void makeDataNetwork();
  void makeLocalDataNetwork(int tileID);
  void makeGlobalDataNetwork();

  void makeCreditNetwork();
  void makeLocalCreditNetwork(int tileID);
  void makeGlobalCreditNetwork();

//==============================//
// Components
//==============================//

private:

  // Flow control at each component's inputs.
  vector<FlowControlIn*> flowControlIn;

  // Flow control at each component's outputs.
  vector<FlowControlOut*> flowControlOut;

  vector<Network*> localDataNetworks;
  vector<Network*> localCreditNetworks;
  Network *globalDataNetwork, *globalCreditNetwork;
  OffChip offChip;  // Should this be in here, or outside?

//==============================//
// Signals (wires)
//==============================//

private:

  // Signals between the network and off-chip component.
  DataSignal        *dataToComponents,    *dataFromComponents;
  CreditSignal      *creditsToComponents, *creditsFromComponents;
  ReadySignal       *validDataToComps,    *validDataFromComps,
                    *validCreditToComps,  *validCreditFromComps;
  ReadySignal       *ackDataToComps,      *ackDataFromComps,
                    *ackCreditToComps,    *ackCreditFromComps;

  // Signals between local and global networks.
  DataSignal        *dataToLocalNet,      *dataFromLocalNet;
  CreditSignal      *creditsToLocalNet,   *creditsFromLocalNet;
  ReadySignal       *validDataToLocal,    *validDataFromLocal,
                    *validCreditToLocal,  *validCreditFromLocal;
  ReadySignal       *localReadyForData,   *localReadyForCredits,
                    *globalReadyForData,  *globalReadyForCredits;

  // Signals between off-chip and its flow control component.
  DataSignal         dataFromOffChip;
  sc_signal<Word>    dataToOffChip;
  sc_signal<int>     creditsFromOffChip;
  ReadySignal        readyToOffChip;

};

#endif /* NETWORKHIERARCHY_H_ */
