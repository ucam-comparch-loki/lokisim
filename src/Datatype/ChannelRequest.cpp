/*
 * ChannelRequest.cpp
 *
 * Current layout:
 *    4 bit port to connect to
 *    16 bit channel ID to return data to
 *    1 bit request type (setup/tear down)
 *
 *    |          spare             | type | channel ID |    port    |
 *     63                           20     19           3           0
 *
 * To set up a channel, send the value:
 *    16*return channel ID + port
 *
 *  Created on: 24 Mar 2010
 *      Author: db434
 */

#include "ChannelRequest.h"

const short startPort = 0;
const short startReturnChannel = startPort + 4;
const short startType = startReturnChannel + 16;
const short end = startType + 1;

int ChannelRequest::getPort() const {
  return getBits(startPort, startReturnChannel-1);
}

int ChannelRequest::getReturnChannel() const {
  return getBits(startReturnChannel, startType-1);
}

int ChannelRequest::getType() const {
  return getBits(startType, end-1);
}

void ChannelRequest::setPort(int val) {
  setBits(startPort, startReturnChannel-1, val);
}

void ChannelRequest::setReturnChannel(int val) {
  setBits(startReturnChannel, startType-1, val);
}

void ChannelRequest::setType(int val) {
  setBits(startType, end-1, val);
}

ChannelRequest::ChannelRequest() : Word() {

}

ChannelRequest::ChannelRequest(const Word& other) : Word(other) {

}

ChannelRequest::ChannelRequest(int port, int returnChannel, int type) :
    Word() {
  setPort(port);
  setReturnChannel(returnChannel);
  setType(type);
}

ChannelRequest::~ChannelRequest() {

}
