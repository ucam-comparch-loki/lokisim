/*
 * MulticastCrossbar.h
 *
 * A crossbar with multicast functionality.
 *
 * Multicast and credits don't mix well, so this crossbar only makes use of
 * link-level flow control.
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#ifndef MULTICASTCROSSBAR_H_
#define MULTICASTCROSSBAR_H_

#include "../../Component.h"
#include "MulticastBus.h"

class ArbiterComponent;

// Should inherit from Crossbar
class MulticastCrossbar: public Component {

//==============================//
// Ports
//==============================//

public:

//  sc_in<bool>   clock;

  DataInput    *dataIn;
  DataOutput   *dataOut;

  CreditInput  *creditsIn;
  CreditOutput *creditsOut;

  ReadyInput   *canSendData;
  ReadyOutput  *canReceiveData;

  // To be removed when network interface is changed.
  ReadyInput   *canSendCredits;
  ReadyOutput  *canReceiveCredits;

//==============================//
// Constructors and destructors
//==============================//

public:

  MulticastCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
                    int outputsPerComponent, int channelsPerOutput, ChannelID startAddr,
                    Dimension size);
  virtual ~MulticastCrossbar();

//==============================//
// Components
//==============================//

private:

  std::vector<MulticastBus*> buses;
  std::vector<ArbiterComponent*> arbiters;

  sc_signal<DataType> *busToMux;
  sc_signal<bool> *newData;

};

#endif /* MULTICASTCROSSBAR_H_ */
