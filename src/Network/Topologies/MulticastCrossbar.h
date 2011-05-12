/*
 * MulticastCrossbar.h
 *
 * A crossbar with multicast functionality.
 *
 * Multicast and credits don't mix well, so this crossbar reduces credits to
 * behave like an acknowledge signal.
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#ifndef MULTICASTCROSSBAR_H_
#define MULTICASTCROSSBAR_H_

#include "Crossbar.h"

class ArbiterComponent;

// Should inherit from Crossbar
class MulticastCrossbar: public Crossbar {

//==============================//
// Ports
//==============================//

public:

// Inherited from Crossbar:
//  sc_in<bool>   clock;
//  DataInput    *dataIn;
//  DataOutput   *dataOut;
//  ReadyInput   *canSendData;
//  ReadyOutput  *canReceiveData;

  CreditInput  *creditsIn;
  ReadyInput   *validCreditIn;
  ReadyOutput  *ackCreditIn;

  CreditOutput *creditsOut;
  ReadyOutput  *validCreditOut;
  ReadyInput   *ackCreditOut;

//==============================//
// Methods
//==============================//

protected:

  virtual void makeBuses(int numBuses, int numArbiters, int channelsPerOutput, const ChannelID& startAddr);

private:

  // TODO: use a demultiplexer component at each credit input, instead of
  // checking all inputs using this method.
  void creditArrived();

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MulticastCrossbar);
  MulticastCrossbar(sc_module_name name, const ComponentID& ID, int inputs, int outputs,
                    int outputsPerComponent, int channelsPerOutput, const ChannelID& startAddr,
                    Dimension size, bool externalConnection=false);
  virtual ~MulticastCrossbar();

//==============================//
// Components
//==============================//

private:

  sc_buffer<CreditType> **creditsToBus;
  sc_signal<ReadyType>  **busReadyCredits;

};

#endif /* MULTICASTCROSSBAR_H_ */
