/*
 * InputCrossbar.cpp
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#include "InputCrossbar.h"
#include "../Datatype/AddressedWord.h"
#include "../Network/FlowControl/FlowControlIn.h"
#include "../Network/Topologies/Crossbar.h"
#include "../Network/UnclockedNetwork.h"
#include "../TileComponents/TileComponent.h"

InputCrossbar::InputCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs) :
    Component(name, ID),
    creditNet("credit", ID, outputs, inputs, inputs, TOTAL_OUTPUT_CHANNELS, 0, Dimension(1.0/CORES_PER_TILE, 0.05)),
    skewedClock("skewed_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.5, sc_core::sc_time(0.25, sc_core::SC_NS), true) {

  Crossbar* data = new Crossbar("data", ID, inputs, outputs, 1, 1, TileComponent::inputChannelID(ID,0), Dimension(1.0/CORES_PER_TILE, 0.05));
  data->initialise();
  dataNet = new UnclockedNetwork("data_net", data);

//  Crossbar* credit = new Crossbar("credit", ID, outputs, inputs, inputs, TOTAL_OUTPUT_CHANNELS, 0, Dimension(1.0/CORES_PER_TILE, 0.05));
//  credit->initialise();
//  creditNet = new UnclockedNetwork("credit_net", credit);

  creditNet.initialise();

  firstInput       = TileComponent::inputChannelID(id, 0);
  numInputs        = inputs;
  numOutputs       = outputs;

  dataIn           = new DataInput[inputs];
  validDataIn      = new ReadyInput[inputs];
  ackDataIn        = new ReadyOutput[inputs];

  dataOut          = new sc_out<Word>[outputs];

  creditsIn        = new sc_in<int>[outputs];

  creditsOut       = new CreditOutput[inputs];
  validCreditOut   = new ReadyOutput[inputs];
  ackCreditOut     = new ReadyInput[inputs];

  dataToBuffer     = new sc_buffer<DataType>[outputs];
  creditsToNetwork = new sc_buffer<CreditType>[outputs];
  readyForData     = new sc_signal<ReadyType>[outputs];
  readyForCredit   = new sc_signal<ReadyType>[outputs];
  validData        = new sc_signal<ReadyType>[outputs];
  validCredit      = new sc_signal<ReadyType>[outputs];

  // FIXME: skewedClock doesn't work here (for only one benchmark), even when
  // it's configured to be identical to the normal clock.
  creditNet.clock(skewedClock);

  for(int i=0; i<inputs; i++) {
    dataNet->dataIn[i](dataIn[i]);
    dataNet->validDataIn[i](validDataIn[i]);
    dataNet->ackDataIn[i](ackDataIn[i]);
    creditNet.dataOut[i](creditsOut[i]);
    creditNet.validDataOut[i](validCreditOut[i]);
    creditNet.ackDataOut[i](ackCreditOut[i]);
  }

  // Create and wire up all flow control units.
  for(int i=0; i<outputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), firstInput+i);
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->dataOut(dataOut[i]);
    fc->creditsIn(creditsIn[i]);

    fc->dataIn(dataToBuffer[i]);
    fc->validDataIn(validData[i]);
    fc->ackDataIn(readyForData[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->validCreditOut(validCredit[i]);
    fc->ackCreditOut(readyForCredit[i]);

    dataNet->dataOut[i](dataToBuffer[i]);
    dataNet->validDataOut[i](validData[i]);
    dataNet->ackDataOut[i](readyForData[i]);
    creditNet.dataIn[i](creditsToNetwork[i]);
    creditNet.validDataIn[i](validCredit[i]);
    creditNet.ackDataIn[i](readyForCredit[i]);
  }
}
InputCrossbar::~InputCrossbar() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;
  delete[] dataOut;
  delete[] creditsIn;
  delete[] creditsOut;
  delete[] validCreditOut;
  delete[] ackCreditOut;

  delete dataNet;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
