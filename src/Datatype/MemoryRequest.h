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
#include "../Typedefs.h"

class MemoryRequest : public Word {

//==============================//
// Methods
//==============================//

public:

  uint32_t address() const;
  uint8_t  operation() const;
  bool isReadRequest() const;
  bool isWriteRequest() const;
  bool isIPKRequest() const;
  bool isSetup() const;
  bool streaming() const;
  bool halfWordAccess() const;
  bool byteAccess() const;

  void incrementAddress();
  void setIPKRequest(bool val); // remove?

private:

  void address(MemoryAddr val);
  void operation(uint8_t val);

//==============================//
// Constructors and destructors
//==============================//

public:

  MemoryRequest();
  MemoryRequest(MemoryAddr address, uint8_t operation);
  MemoryRequest(const Word& other);

//==============================//
// Local state
//==============================//

  // The memory operation being performed.
  enum MemoryOp {
    NONE,
    SETUP=0,  // Currently have no way to tag a value as a setup request
    LOAD_W, LOAD_HW, LOAD_B,
    IPK_READ,
    STORE_W, STORE_HW, STORE_B,
    STADDR, STBADDR
  };

};

#endif /* MEMORYREQUEST_H_ */
