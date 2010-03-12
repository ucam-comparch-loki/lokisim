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
  sc_out<bool> *flowControl, stallOut;

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
  void updateStall();
  void checkWaiting(int channelEnd);
  void read(short inChannel, short outChannel);

/* Local state */
  BufferArray<Word> buffers;
  int               waiting1, waiting2; // Channel ends we're waiting for data on
  bool              stallValue;

/* Signals (wires) */
  sc_buffer<bool>   readFromBuffer, wroteToBuffer;
  sc_signal<bool>   updateStall1, updateStall2;

};

#endif /* RECEIVECHANNELENDTABLE_H_ */
