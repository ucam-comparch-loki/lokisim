/*
 * Address.h
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef ADDRESS_H_
#define ADDRESS_H_

#include "Word.h"

class Address: public Word {

//==============================//
// Methods
//==============================//

public:

  uint16_t getAddress() const;
  uint16_t getChannelID() const;
  bool getReadBit() const;

  uint32_t getLowestBits(const int limit) const;

  void addOffset(const int offset);

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, const Address& v) {
    os << "(" << v.getAddress() << ", " << v.getChannelID() << ")";
    return os;
  }

private:

  void setAddress(const uint16_t addr);
  void setChannelID(const uint16_t channelID);
  void setRWBit(const bool read);

//==============================//
// Constructors and destructors
//==============================//

public:

  Address();
  Address(const Word& other);
  Address(const uint16_t addr, const uint16_t channelID);
  virtual ~Address();
};

#endif /* ADDRESS_H_ */
