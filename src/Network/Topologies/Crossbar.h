/*
 * Crossbar.h
 *
 * A crossbar without multicast functionality.
 *
 *  Created on: 21 Mar 2011
 *      Author: db434
 */

#ifndef CROSSBAR_H_
#define CROSSBAR_H_

#include "../Network.h"
#include "Bus.h"

class ArbiterComponent;

class Crossbar: public Network {

//==============================//
// Ports
//==============================//

// Inherited from Network:
//
//  sc_in<bool>   clock;
//
//  DataInput    *dataIn;
//  DataOutput   *dataOut;
//
//  ReadyInput   *canSendData;
//  ReadyOutput  *canReceiveData;

//==============================//
// Constructors and destructors
//==============================//

public:
  Crossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
           int outputsPerComponent, int channelsPerOutput, ChannelID startAddr,
           Dimension size, bool externalConnection = false);
  virtual ~Crossbar();

//==============================//
// Components
//==============================//

private:

  std::vector<Bus*>              buses;
  std::vector<ArbiterComponent*> arbiters;

  sc_signal<DataType>           *busToMux;
  sc_signal<ReadyType>          *newData;
  sc_signal<ReadyType>          *readData;

};

#endif /* CROSSBAR_H_ */
