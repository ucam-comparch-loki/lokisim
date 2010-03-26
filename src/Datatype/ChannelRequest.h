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

  int getPort() const;
  int getReturnChannel() const;
  int getType() const;

private:

  void setPort(int val);
  void setReturnChannel(int val);
  void setType(int val);

//==============================//
// Constructors and destructors
//==============================//

public:

  ChannelRequest();
  ChannelRequest(const Word& other);
  ChannelRequest(int port, int returnChannel, int type);
  virtual ~ChannelRequest();

//==============================//
// Local state
//==============================//

public:

  enum RequestType {SETUP, TEARDOWN};

};

#endif /* CHANNELREQUEST_H_ */
