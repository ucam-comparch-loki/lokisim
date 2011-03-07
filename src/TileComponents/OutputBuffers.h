/*
 * OutputBuffers.h
 *
 * The output buffers of a TileComponent (core or memory), and the arbitration
 * to decide which buffers should be allowed to send data on a particular cycle.
 *
 *  Created on: 2 Mar 2011
 *      Author: db434
 */

#ifndef OUTPUTBUFFERS_H_
#define OUTPUTBUFFERS_H_

#include "../Component.h"
#include "../Arbitration/ArbiterBase.h"
#include "BufferWithCounter.h"

class AddressedWord;

class OutputBuffers: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>            clock;

  sc_in<AddressedWord>  *dataIn;
  sc_in<AddressedWord>  *creditsIn;
  sc_out<AddressedWord> *dataOut;
  sc_out<bool>          *flowControlOut;
  sc_out<bool>          *emptyBuffer;

//==============================//
// Methods
//==============================//

private:

  void sendAll();
  void sendSingle(int bufferIndex, int output);
  bool canSend(int bufferIndex) const;
  void receivedCredits();

  RequestList& allRequests() const;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(OutputBuffers);
  OutputBuffers(sc_module_name name, int inputs, int outputs,
                int buffSize = CHANNEL_END_BUFFER_SIZE);
  virtual ~OutputBuffers();

//==============================//
// Local state
//==============================//

private:

  uint numInputs, numOutputs;

  ArbiterBase* arbiter;

  std::vector<BufferWithCounter<AddressedWord>*> buffers;

  sc_buffer<bool> *doRead, *creditSig;
  sc_buffer<AddressedWord> *dataFromBuffers;

};

#endif /* OUTPUTBUFFERS_H_ */
