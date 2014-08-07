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
#include "../Network/Topologies/InstantCrossbar.h"
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

  ClockInput   clock;

  // A slight hack to improve simulation speed. We need to skew the times that
  // this network sends and receives data so data can get through this small
  // network and the larger tile network in one cycle.
  // In practice, these would probably be implemented as delays in the small
  // network.
  ClockInput   creditClock;

  LokiVector<DataInput>     iData;
  LokiVector<ReadyOutput>   oReady;

  LokiVector<sc_out<Word> > oData;

  LokiVector<ReadyInput>    iFlowControl;
  LokiVector<ReadyInput>    iDataConsumed;

  LokiVector<CreditOutput>  oCredit;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(InputCrossbar);
  InputCrossbar(sc_module_name name, const ComponentID& ID);
  virtual ~InputCrossbar();

//==============================//
// Methods
//==============================//

private:

  // Monitor the input ports for new data, and pass it to the corresponding
  // flow control unit when it arrives.
  void newData(PortIndex input);

  // Write data to the chosen output buffer. The input port to get the data
  // from is stored in the dataSource vector.
  void writeToBuffer(ChannelIndex output);

  // Update the flow control signal for a particular buffer. I would prefer to
  // use the signal straight from the buffer, but this does not seem possible.
  void updateFlowControl(ChannelIndex input);

//==============================//
// Local state
//==============================//

private:

  static const unsigned int numInputs, numOutputs;
  ChannelID firstInput;

  std::vector<FlowControlIn*> flowControl;

  InstantCrossbar          creditNet;

  ReadySignal              constantHigh;

  LokiVector<DataSignal>   dataToBuffer;
  LokiVector<CreditSignal> creditsToNetwork;

  // An event for each output port which is triggered when there is data to
  // put into its buffer.
  LokiVector<sc_event>     sendData;

  // The source port of data to be written into each output buffer.
  std::vector<PortIndex>   dataSource;

};

#endif /* INPUTCROSSBAR_H_ */
