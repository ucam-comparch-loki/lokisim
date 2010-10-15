/*
 * ChannelRequest.h
 *
 * A datatype used to setup and tear down channels between clusters and
 * memories.
 *
 *  Created on: 24 Mar 2010
 *      Author: db434
 */

#ifndef CHANNELREQUEST_H_
#define CHANNELREQUEST_H_

#include "Word.h"

class ChannelRequest: public Word {

//==============================//
// Methods
//==============================//

public:

  uint8_t  port() const;
  uint16_t returnChannel() const;
  uint8_t  type() const;

private:

  void port(uint8_t val);
  void returnChannel(uint16_t val);
  void type(uint8_t val);

//==============================//
// Constructors and destructors
//==============================//

public:

  ChannelRequest();
  ChannelRequest(const Word& other);
  ChannelRequest(uint8_t port, uint16_t returnChannel, uint8_t type);
  virtual ~ChannelRequest();

//==============================//
// Local state
//==============================//

public:

  enum RequestType {SETUP, TEARDOWN};

};

#endif /* CHANNELREQUEST_H_ */
