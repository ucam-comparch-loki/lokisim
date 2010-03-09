/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"
#include "../Datatype/Instruction.h"
#include "../Datatype/MemoryRequest.h"

/* Look through all inputs for new data. Determine whether this data is the
 * start of a new transaction or the continuation of an existing one. Then
 * carry out the first/next step of the transaction. */
void MemoryMat::doOp() {

  for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {

    if(in[i].event()) {
      // If there isn't an existing transaction, this must be the start of one
      if(!transactions[i].isActive()) {
        if(DEBUG) cout << "Received new memory request at input " << i << endl;
        transactions[i] = static_cast<MemoryRequest>(in[i].read());
        if(transactions[i].isReadRequest()) read(i);
      }
      // If there is an existing transaction, this must be data to be written
      else write(in[i].read(), i);
    }
    // If there is a read transaction in progress, send the next value
    else if(transactions[i].isActive() && transactions[i].isReadRequest()) {
      read(i);
    }
    else {
      flowControlOut[i].write(true);
    }

  }

}

/* Carry out a read for the transaction at input "position". */
void MemoryMat::read(int position) {
  if(flowControlIn[position].read()) {
    int addr = transactions[position].getMemoryAddress();
    short returnAddr = transactions[position].getReturnAddress();
    AddressedWord* aw = new AddressedWord(data.read(addr), returnAddr);
    out[position].write(*aw);

    transactions[position].decrementNumOps();
    transactions[position].incrementAddress();

    // If we have reached the end of the packet, deactivate the request
    if(transactions[position].isIPKRequest() &&
       (static_cast<Instruction>(data.read(addr))).endOfPacket()) {
      transactions[position].setIPKRequest(false);
    }

    // Don't allow new transactions until the current one finishes
    if(transactions[position].isActive()) flowControlOut[position].write(false);
    else flowControlOut[position].write(true);

    if(DEBUG) cout<<"Read "<<data.read(addr)<<" from address "<<addr<<endl;
  }
}

/* Carry out a write for the transaction at input "position". */
void MemoryMat::write(Word w, int position) {
  int addr = transactions[position].getMemoryAddress();
  data.write(w, addr);

  transactions[position].decrementNumOps();
  transactions[position].incrementAddress();

  flowControlOut[position].write(true);   // Is this necessary?

  if(DEBUG) cout << "Wrote " << w << " to address " << addr << endl;
}

/* Initialise the contents of this memory to the Words in the given vector. */
void MemoryMat::storeData(std::vector<Word>& data) {
  for(unsigned int i=0; i<data.size(); i++) {
    this->data.write(data[i], i);
  }
}

MemoryMat::MemoryMat(sc_module_name name, int ID) :
    TileComponent(name, ID),
    data(MEMORY_SIZE) {

  transactions = new MemoryRequest[NUM_CLUSTER_INPUTS];

// Register methods
  SC_METHOD(doOp);
  /*for(int i=0; i<NUM_CLUSTER_INPUTS; i++)*/ sensitive << clock.pos();//in[i];
  dont_initialize();

  end_module(); // Needed because we're using a different constructor

}

MemoryMat::~MemoryMat() {

}
