/*
 * MemoryMat.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "MemoryMat.h"

/* Look through all inputs for new data. Determine whether this data is the
 * start of a new transaction or the continuation of an existing one. Then
 * carry out the first/next step of the transaction. */
void MemoryMat::doOp() {
  for(int i=0; i<NUM_CLUSTER_INPUTS; i++) {
    if(in[i].event()) {
      if(!transactions[i].isActive()) {
        transactions[i] = static_cast<MemoryRequest>(in[i].read());
        if(transactions[i].isReadRequest()) read(i);
      }
      else write(in[i].read(), i);
    }
    else if(transactions[i].isActive() && transactions[i].isReadRequest()) {
      read(i);
    }
  }
}

void MemoryMat::read(int position) {
  if(flowControlIn[position].read()) {
    int addr = transactions[position].getStartAddress();
    short returnAddr = transactions[position].getReturnAddress();
    AddressedWord* aw = new AddressedWord(data.read(addr), returnAddr);
    out[position].write(*aw);

    transactions[position].decrementNumOps();
    transactions[position].incrementAddress();

    // flow control = false, except at end of transaction?

    if(DEBUG) cout<<"Read "<<data.read(addr)<<" from address "<<addr<<endl;
  }
}

void MemoryMat::write(Word w, int position) {
  int addr = transactions[position].getStartAddress();
  data.write(w, addr);

  transactions[position].decrementNumOps();
  transactions[position].incrementAddress();

  if(DEBUG) cout << "Wrote " << w << " to address " << addr << endl;
}

MemoryMat::MemoryMat(sc_module_name name, int ID) :
    TileComponent(name, ID),
    data(MEMORY_SIZE) {

  transactions = new MemoryRequest[NUM_CLUSTER_INPUTS];

// Register methods
  SC_METHOD(doOp);
  for(int i=0; i<NUM_CLUSTER_INPUTS; i++) sensitive << clock.pos();//in[i];
  dont_initialize();

  end_module(); // Needed because we're using a different constructor

}

MemoryMat::~MemoryMat() {

}
