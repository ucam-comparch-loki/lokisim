/*
 * MemoryRequest.h
 *
 *  Created on: 3 Mar 2010
 *      Author: db434
 */

#ifndef MEMORYREQUEST_H_
#define MEMORYREQUEST_H_

#include "Word.h"

class MemoryRequest : public Word {
public:
/* Methods */
  unsigned short getMemoryAddress() const;
  unsigned short getReturnAddress() const;
  unsigned short getNumOps() const;
  bool isReadRequest() const;
  bool isIPKRequest() const;
  bool isActive() const;

  void decrementNumOps();
  void incrementAddress();
  void setIPKRequest(bool val);

/* Constructors and destructors */
  MemoryRequest();
  MemoryRequest(short startAddr, short returnAddr, short numOps, bool read);
  MemoryRequest(const Word& other);
  virtual ~MemoryRequest();

private:
/* Methods */
  void setMemoryAddress(short val);
  void setReturnAddress(short val);
  void setNumOps(short val);
  void setRWBit(bool val);

};

#endif /* MEMORYREQUEST_H_ */
