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
#include "../Datatype/AddressedWord.h"

using std::vector;

class FlowControlIn;
class FlowControlOut;
class Network;

typedef AddressedWord         CreditType;     // May be a bool one day
typedef AddressedWord         DataType;
typedef bool                  ReadyType;

typedef sc_buffer<CreditType> CreditSignal;
typedef sc_buffer<DataType>   DataSignal;
typedef sc_signal<ReadyType>  ReadySignal;

typedef sc_in<DataType>       DataInput;
typedef sc_out<DataType>      DataOutput;
typedef sc_in<ReadyType>      ReadyInput;
typedef sc_out<ReadyType>     ReadyOutput;
typedef sc_in<CreditType>     CreditInput;
typedef sc_out<CreditType>    CreditOutput;

class NetworkHierarchy : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  // Data received from each output of each networked component.
  DataInput    *dataIn;
  ReadyOutput  *canReceiveData;

  // Data sent to each networked component (after having its address removed
  // by the flow control components).
  DataOutput   *dataOut;
  ReadyInput   *canSendData;

  // Flow control information received from each input of each component.
  CreditInput  *creditsIn;
  ReadyOutput  *canReceiveCredit;

  // A signal telling each inputput whether it is allowed to send a credit.
  CreditOutput *creditsOut;
  ReadyInput   *canSendCredit;


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
  DataSignal        *dataToComponents,    *dataFromComponents;
  CreditSignal      *creditsToComponents, *creditsFromComponents;
  ReadySignal       *compReadyForData,    *compReadyForCredits,
                    *readyForData,        *readyForCredits;

  // Global network
  DataSignal        *dataToLocalNet,      *dataFromLocalNet;
  CreditSignal      *creditsToLocalNet,   *creditsFromLocalNet;
  ReadySignal       *localReadyForData,   *localReadyForCredits,
                    *globalReadyForData,  *globalReadyForCredits;

  // Off-chip
  DataSignal         dataFromOffChip;
  sc_signal<Word>    dataToOffChip;
  sc_signal<int>     creditsFromOffChip;
  ReadySignal        readyToOffChip;

};

#endif /* NETWORKHIERARCHY_H_ */
