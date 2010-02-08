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

  void setAddress(unsigned int addr);
  void setChannelID(unsigned int channelID);
  void setRWBit(bool read);

public:
  unsigned int getAddress() const;
  unsigned int getChannelID() const;
  bool getReadBit() const;

  unsigned int getLowestBits(int limit) const;

  void addOffset(int offset);

  Address();
  Address(const Word& other);
  Address(int addr, int channelID);
  virtual ~Address();
};

#endif /* ADDRESS_H_ */
