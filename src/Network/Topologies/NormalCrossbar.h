/*
 * NormalCrossbar.h
 *
 * A crossbar without multicast functionality. To be renamed "Crossbar" when
 * the existing class of that name is obsolete.
 *
 *  Created on: 21 Mar 2011
 *      Author: db434
 */

#ifndef NORMALCROSSBAR_H_
#define NORMALCROSSBAR_H_

#include "../../Component.h"
#include "Bus.h"

class ArbiterComponent;

class NormalCrossbar: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  DataInput    *dataIn;
  DataOutput   *dataOut;

  ReadyInput   *canSendData;
  ReadyOutput  *canReceiveData;

//==============================//
// Constructors and destructors
//==============================//

public:
  NormalCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs,
                 int outputsPerComponent, int channelsPerOutput, ChannelID startAddr);
  virtual ~NormalCrossbar();

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

#endif /* NORMALCROSSBAR_H_ */
