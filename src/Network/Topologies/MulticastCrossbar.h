/*
 * MulticastCrossbar.h
 *
 *  Created on: 9 Mar 2011
 *      Author: db434
 */

#ifndef MULTICASTCROSSBAR_H_
#define MULTICASTCROSSBAR_H_

#include "../../Component.h"
#include "Bus.h"

class ArbiterComponent;

class MulticastCrossbar: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  DataInput    *dataIn;
  DataOutput   *dataOut;

  CreditInput  *creditsIn;
  CreditOutput *creditsOut;

  ReadyInput   *readyIn;
  ReadyOutput  *readyOut;

  // To be removed when network interface is changed.
  ReadyInput   *readyInCredits;
  ReadyOutput  *readyOutCredits;

//==============================//
// Constructors and destructors
//==============================//

public:

  MulticastCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
                    int outputsPerComponent, int channelsPerOutput, ChannelID startAddr);
  virtual ~MulticastCrossbar();

//==============================//
// Components
//==============================//

private:

  std::vector<Bus*> buses;
  std::vector<ArbiterComponent*> arbiters;

  sc_signal<DataType> *busToMux;
  sc_signal<bool> *newData;

};

#endif /* MULTICASTCROSSBAR_H_ */
