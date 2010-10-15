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

  uint32_t address() const;
  uint8_t  operation() const;
  bool isReadRequest() const;
  bool isIPKRequest() const;

  void incrementAddress();
  void setIPKRequest(bool val); // remove?

private:

  void address(uint32_t val);
  void operation(uint8_t val);

//==============================//
// Constructors and destructors
//==============================//

public:

  MemoryRequest();
  MemoryRequest(uint32_t address, uint8_t operation);
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
    STADDR, STBADDR,
    // SETUP, ? used to tell a memory where to send results back to
    NONE
  };

};

#endif /* MEMORYREQUEST_H_ */
