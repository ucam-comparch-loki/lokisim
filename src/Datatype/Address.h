/*
 * Address.h
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef ADDRESS_H_
#define ADDRESS_H_

#include "Word.h"
#include "../Typedefs.h"

class Address: public Word {

//==============================//
// Methods
//==============================//

public:

  // Return the memory address being referred to.
  MemoryAddr address() const;

  // Return the channel ID of the memory we want the data from.
  ChannelID channelID() const;

  // Tell whether we are accessing the memory address to read or write it.
  // 1 = read, 0 = write.
  bool     readBit() const;

  Address& addOffset(const int offset);

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, const Address& v) {
    os << "(" << v.address() << ", " << v.channelID() << ")";
    return os;
  }

private:

  void address(const MemoryAddr addr);
  void channelID(const ChannelID channelID);
  void setRWBit(const bool read);

//==============================//
// Constructors and destructors
//==============================//

public:

  Address();
  Address(const Word& other);
  Address(const MemoryAddr addr, const ChannelID channelID);
  virtual ~Address();
};

#endif /* ADDRESS_H_ */
