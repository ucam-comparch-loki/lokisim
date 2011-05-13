/*
 * InputCrossbar.h
 *
 * A crossbar between all input ports of a core, and all input buffers. There
 * will usually be many more buffers than ports.
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#ifndef INPUTCROSSBAR_H_
#define INPUTCROSSBAR_H_

#include "../Component.h"
#include "../Network/Topologies/Crossbar.h"
#include "../Network/NetworkTypedefs.h"

class AddressedWord;
class FlowControlIn;
class UnclockedNetwork;
class Word;

class InputCrossbar: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  DataInput    *dataIn;
  ReadyInput   *validDataIn;
  ReadyOutput  *ackDataIn;

  sc_out<Word> *dataOut;

  sc_in<int>   *creditsIn;

  CreditOutput *creditsOut;
  ReadyOutput  *validCreditOut;
  ReadyInput   *ackCreditOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  InputCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs);
  virtual ~InputCrossbar();

//==============================//
// Local state
//==============================//

private:

  ChannelID firstInput;
  int numInputs, numOutputs;

  std::vector<FlowControlIn*> flowControl;

  Crossbar               creditNet;
  UnclockedNetwork*      dataNet;
//  UnclockedNetwork*      creditNet;

  sc_buffer<DataType>   *dataToBuffer;
  sc_buffer<CreditType> *creditsToNetwork;
  sc_signal<ReadyType>  *readyForData, *readyForCredit;
  sc_signal<ReadyType>  *validData, *validCredit;

  // Skew the clock by 1/4 of a cycle for the credit network. This is done
  // because this small credit network is followed by the main, tile credit
  // network, and we want the credit to get to the main network before the
  // main network triggers the sending of credits.
  sc_core::sc_clock      skewedClock;

};

#endif /* INPUTCROSSBAR_H_ */
