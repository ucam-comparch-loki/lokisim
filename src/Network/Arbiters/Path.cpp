/*
 * Path.cpp
 *
 *  Created on: 16 Nov 2010
 *      Author: db434
 */

#include "Path.h"

ChannelIndex Path::source() const {
  return source_;
}

ChannelIndex Path::destination() const {
  return destination_;
}

AddressedWord Path::data() const {
  return data_;
}

Path& Path::operator= (const Path& other) {
  source_ = other.source_;
  destination_ = other.destination_;
  data_ = other.data_;
  return *this;
}

Path::Path(const Path& other) {
  *this = other;
}

Path::Path(ChannelIndex source, ChannelIndex destination, const AddressedWord& data) {
  source_ = source;
  destination_ = destination;
  data_ = data;
}

Path::~Path() {

}
