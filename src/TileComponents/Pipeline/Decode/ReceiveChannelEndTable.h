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

#include "../../../Component.h"
#include "../../../Datatype/Word.h"
#include "../../../Datatype/Address.h"
#include "../../../Datatype/Data.h"
#include "../../../Memory/BufferArray.h"

class ReceiveChannelEndTable: public Component {

public:
/* Ports */
  sc_in<bool>   clock;
  sc_in<Word>  *fromNetwork;
  sc_in<short>  fromDecoder1, fromDecoder2;
  sc_out<Data>  toALU1, toALU2;
  sc_out<bool> *flowControl;

/* Constructors and destructors */
  SC_HAS_PROCESS(ReceiveChannelEndTable);
  ReceiveChannelEndTable(sc_module_name name);
  virtual ~ReceiveChannelEndTable();

private:
/* Methods */
  void receivedInput();
  void read1();
  void read2();
  void updateFlowControl();
  void read(short inChannel, short outChannel);

/* Local state */
  BufferArray<Word> buffers;
  sc_buffer<bool> readFromBuffer, wroteToBuffer;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
