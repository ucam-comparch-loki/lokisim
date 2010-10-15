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

uint8_t ChannelRequest::port() const {
  return getBits(startPort, startReturnChannel-1);
}

uint16_t ChannelRequest::returnChannel() const {
  return getBits(startReturnChannel, startType-1);
}

uint8_t ChannelRequest::type() const {
  return getBits(startType, end-1);
}

void ChannelRequest::port(uint8_t val) {
  setBits(startPort, startReturnChannel-1, val);
}

void ChannelRequest::returnChannel(uint16_t val) {
  setBits(startReturnChannel, startType-1, val);
}

void ChannelRequest::type(uint8_t val) {
  setBits(startType, end-1, val);
}

ChannelRequest::ChannelRequest() : Word() {

}

ChannelRequest::ChannelRequest(const Word& other) : Word(other) {

}

ChannelRequest::ChannelRequest(uint8_t portID, uint16_t channelID, uint8_t reqType) :
    Word() {
  port(portID);
  returnChannel(channelID);
  type(reqType);
}

ChannelRequest::~ChannelRequest() {

}
