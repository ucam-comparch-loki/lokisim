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

class AddressedWord;
class FlowControlIn;
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

  SC_HAS_PROCESS(InputCrossbar);
  InputCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs);
  virtual ~InputCrossbar();

//==============================//
// Methods
//==============================//

private:

  void dataArrived();
  void sendData();

//==============================//
// Local state
//==============================//

private:

  ChannelID firstInput;
  int numInputs, numOutputs;

  std::vector<FlowControlIn*> flowControl;
  Crossbar dataXbar;
  Crossbar creditXbar;

  sc_buffer<DataType>   *dataToBuffer;
  sc_buffer<CreditType> *creditsToNetwork;
  sc_signal<ReadyType>  *readyForData, *readyForCredit;
  sc_signal<ReadyType>  *validData, *validCredit;

  // This signal behaves like a clock for the data network: we generate a
  // negative edge when we want the network to send data.
  // The equivalent credit network can use the ordinary clock, so this is not
  // needed.
  sc_signal<bool> sendDataSig;
  sc_core::sc_event newData;

};

#endif /* INPUTCROSSBAR_H_ */
