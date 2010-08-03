/*
 * MemoryRequest.h
 *
 * Request to set up a channel with a memory. Specify the type of operation
 * to be performed and the address in memory to perform it.
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#ifndef MEMORYREQUEST_H_
#define MEMORYREQUEST_H_

#include "Word.h"

class MemoryRequest : public Word {

//==============================//
// Methods
//==============================//

public:

  unsigned int getAddress() const;
  unsigned short getOperation() const;
  bool isReadRequest() const;
  bool isIPKRequest() const;

  void incrementAddress();
  void setIPKRequest(bool val); // remove?

private:

  void setAddress(int val);
  void setOperation(short val);

//==============================//
// Constructors and destructors
//==============================//

public:

  MemoryRequest();
  MemoryRequest(short address, short operation);
  MemoryRequest(const Word& other);
  virtual ~MemoryRequest();

//==============================//
// Local state
//==============================//

  // The memory operation being performed.
  enum MemoryOp {
    LOAD, LOAD_B,
    IPK_READ,
    STORE, STORE_B,
    STADDR, STBADDR
  };
  // none?

};

#endif /* MEMORYREQUEST_H_ */
