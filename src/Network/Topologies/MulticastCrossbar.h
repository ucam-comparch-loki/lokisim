/*
 * MulticastCrossbar.h
 *
 * A crossbar with multicast functionality.
 *
 * Multicast and credits don't mix well, so this crossbar reduces credits to
 * behave like an acknowledge signal. This means that the destination(s) for
 * data can't be changed if there are still outstanding credits.
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#ifndef MULTICASTCROSSBAR_H_
#define MULTICASTCROSSBAR_H_

#include "Crossbar.h"

class UnclockedNetwork;

class MulticastCrossbar: public Crossbar {

//==============================//
// Ports
//==============================//

public:

// Inherited from Crossbar:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//  ReadyInput   *validDataIn;
//  ReadyOutput  *ackDataIn;
//
//  DataOutput   *dataOut;
//  ReadyOutput  *validDataOut;
//  ReadyInput   *ackDataOut;

  CreditInput  *creditsIn;
  ReadyInput   *validCreditIn;
  ReadyOutput  *ackCreditIn;

  CreditOutput *creditsOut;
  ReadyOutput  *validCreditOut;
  ReadyInput   *ackCreditOut;

//==============================//
// Methods
//==============================//

public:

  // The input port to this network which comes from the next level of network
  // hierarchy (or off-chip).
  CreditInput&  externalCreditIn() const;

  // The output port of this network which goes to the next level of network
  // hierarchy (or off-chip).
  CreditOutput& externalCreditOut() const;

  ReadyInput&   externalValidCreditIn() const;
  ReadyOutput&  externalValidCreditOut() const;

  ReadyInput&   externalAckCreditIn() const;
  ReadyOutput&  externalAckCreditOut() const;

private:

  virtual void makeBuses();

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MulticastCrossbar);
  MulticastCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
                    int outputsPerComponent, int channelsPerOutput, ChannelID firstOutput,
                    int inputsPerComponent, int channelsPerInput, ChannelID firstInput,
                    Dimension size, bool externalConnection=false);
  virtual ~MulticastCrossbar();

//==============================//
// Components
//==============================//

private:

  // A very small crossbar for each component, taking credits from the
  // component's inputs to the appropriate data bus. Credits need to go to the
  // data bus because that is where the multicast takes place, so it is where
  // combination of credits occurs too.
  std::vector<UnclockedNetwork*> creditCrossbars;

  CreditSignal **creditsToBus;
  ReadySignal  **validCreditToBus;
  ReadySignal  **ackCreditToBus;

  bool newCredits;

};
#endif /* MULTICASTCROSSBAR_H_ */
