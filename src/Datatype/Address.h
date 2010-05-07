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

  short getAddress() const;
  unsigned short getChannelID() const;
  bool getReadBit() const;

  unsigned int getLowestBits(int limit) const;

  void addOffset(int offset);

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, const Address& v) {
    os << "(" << v.getChannelID() << ", " << v.getAddress() << ")";
    return os;
  }

private:

  void setAddress(short addr);
  void setChannelID(unsigned short channelID);
  void setRWBit(bool read);

//==============================//
// Constructors and destructors
//==============================//

public:

  Address();
  Address(const Word& other);
  Address(int addr, int channelID);
  virtual ~Address();
};

#endif /* ADDRESS_H_ */
