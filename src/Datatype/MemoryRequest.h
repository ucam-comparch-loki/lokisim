/*
 * MemoryRequest.h
 *
 * Request to set up a channel with a memory. Specify the type of operation
 * to be performed, and how long the channel will be required.
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

  unsigned short getMemoryAddress() const;
  unsigned short getReturnAddress() const;
  unsigned short getNumOps() const;
  bool isReadRequest() const;
  bool isIPKRequest() const;
  bool isActive() const;

  void decrementNumOps();
  void incrementAddress();
  void setIPKRequest(bool val);

private:

  void setMemoryAddress(short val);
  void setReturnAddress(short val);
  void setNumOps(short val);
  void setRWBit(bool val);

//==============================//
// Constructors and destructors
//==============================//

public:

  MemoryRequest();
  MemoryRequest(short startAddr, short returnAddr, short numOps, bool read);
  MemoryRequest(const Word& other);
  virtual ~MemoryRequest();

};

#endif /* MEMORYREQUEST_H_ */
