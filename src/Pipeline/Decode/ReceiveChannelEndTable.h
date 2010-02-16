/*
 * ReceiveChannelEndTable.h
 *
 * Component containing a vector of buffers storing data from input channels.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef RECEIVECHANNELENDTABLE_H_
#define RECEIVECHANNELENDTABLE_H_

#include "../../Component.h"
#include "../../Datatype/Word.h"
#include "../../Datatype/Address.h"
#include "../../Datatype/Data.h"
#include "../../Memory/Buffer.h"

class ReceiveChannelEndTable: public Component {

/* Local state */
  vector<Buffer<Word> > buffers;

/* Methods */
  void receivedInput1();
  void receivedInput2();
  void read1();
  void read2();
  //void receivedInput();
  void read(short inChannel, short outChannel);

public:
/* Ports */
  sc_in<Word>   fromNetwork1, fromNetwork2;
  sc_in<short>  fromDecoder1, fromDecoder2;
  sc_out<Data>  toALU1, toALU2;

  // Flow control?

/* Constructors and destructors */
  SC_HAS_PROCESS(ReceiveChannelEndTable);
  ReceiveChannelEndTable(sc_core::sc_module_name name, int ID);
  virtual ~ReceiveChannelEndTable();

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
