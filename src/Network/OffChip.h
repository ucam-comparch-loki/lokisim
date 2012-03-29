/*
 * OffChip.h
 *
 * A component representing off-chip resources.
 *
 * Currently, it is empty, and just absorbs all data sent to it.
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef OFFCHIP_H_
#define OFFCHIP_H_

#include "../Component.h"
#include "NetworkTypedefs.h"

class AddressedWord;
class Word;

class OffChip: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<Word> dataIn;
  DataOutput  dataOut;
  ReadyInput  readyIn;
  ReadyOutput readyOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(OffChip);
  OffChip(sc_module_name name);
  virtual ~OffChip();

//==============================//
// Methods
//==============================//

private:

  void newData();

};

#endif /* OFFCHIP_H_ */
