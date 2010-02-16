/*
 * Request.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: db434
 */

#include "Request.h"

short Request::getReturnID() const {
  return getBits(0,15);
}

short Request::getNumFlits() const {
  return getBits(16,31);
}

bool Request::isReadRequest() const {
  return getBits(32,32);
}

/* returnID = the channel ID to send the response to this request to
 * numFlits = the number of flits (~= packets) the requester would like sent */
Request::Request(short returnID, short numFlits = 1, bool readRequest = false) {
  setBits(0, 15, returnID);
  setBits(16, 31, numFlits);
  setBits(32, 32, readRequest ? 1 : 0);
}

Request::Request(const Word& other) : Word(other) {

}

Request::~Request() {

}
