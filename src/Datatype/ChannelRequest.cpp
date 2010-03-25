/*
 * ChannelRequest.cpp
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

int ChannelRequest::setPort(int val) {
  setBits(startPort, startReturnChannel-1, val);
}

int ChannelRequest::setReturnChannel(int val) {
  setBits(startReturnChannel, startType-1, val);
}

int ChannelRequest::setType(int val) {
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
