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
#include "OffChip.h"
#include "../flag_signal.h"
#include "../Datatype/AddressedWord.h"

using std::vector;

class FlowControlIn;
class FlowControlOut;
class Network;
class TileComponent;

typedef AddressedWord CreditType;     // May be a bool one day
typedef AddressedWord DataType;
typedef bool ReadyType;
typedef flag_signal<CreditType> CreditSignal;
typedef flag_signal<DataType> DataSignal;
typedef sc_signal<ReadyType> ReadySignal;

class NetworkHierarchy : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>         clock;

  // Data received from each output of each networked component.
  sc_in<DataType>    *dataIn;

  // Data sent to each networked component (after having its address removed
  // by the flow control components).
  sc_out<AddressedWord> *dataOut;

  // Flow control information received from each input of each component.
  sc_in<CreditType>  *creditsIn;

  // A signal telling each output whether it is allowed to send more data.
  sc_out<ReadyType>  *canReceiveData;

  sc_out<CreditType> *creditsOut;
  sc_in<ReadyType>   *canSendCredit;

  sc_in<ReadyType>   *canSendData;
  sc_out<ReadyType>  *canReceiveCredit;

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

  vector<Network*> localDataNetworks, localCreditNetworks;
  Network *globalDataNetwork, *globalCreditNetwork;
  OffChip offChip;  // Should this be in here, or outside?

//==============================//
// Signals (wires)
//==============================//

private:

  // Local network
  DataSignal        *dataToComponents, *dataFromComponents;
  CreditSignal      *creditsToComponents, *creditsFromComponents;
  ReadySignal       *compReadyForData, *compReadyForCredits,
                    *readyForData, *readyForCredits;

  // Global network
  DataSignal        *dataToLocalNet, *dataFromLocalNet;
  CreditSignal      *creditsToLocalNet, *creditsFromLocalNet;
  ReadySignal       *localReadyForData, *localReadyForCredits,
                    *globalReadyForData, *globalReadyForCredits;

  // Off-chip
  DataSignal         dataFromOffChip;
  flag_signal<Word>  dataToOffChip;
  flag_signal<int>   creditsFromOffChip;
  ReadySignal        readyToOffChip;

};

#endif /* NETWORKHIERARCHY_H_ */
