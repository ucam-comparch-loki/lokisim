/*
 * Packet.h
 *
 * A collection of Flits to be sent or received over the network.
 *
 *  Created on: 3 Sep 2014
 *      Author: db434
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <assert.h>
#include <vector>
#include "../../Datatype/Flit.h"

using std::vector;

template <typename T>
class Packet {

//==============================//
// Constructors
//==============================//

public:

  Packet<T>() {
    flitsSent_ = 0;
    flitsReceived_ = 0;
  }

  Packet<T>(const vector<Flit<T> >& data) {
    flitsSent_ = 0;
    flitsReceived_ = 0;
    data_ = data;

    // Ensure that final flit is marked as end-of-packet, but no others.
    for (int i=0; i<data.size()-1; i++)
      assert(!data[i].endOfPacket());
    data_.back().setEndOfPacket(true);
  }

//==============================//
// Methods
//==============================//

public:

  // The number of flits in the packet. If the packet is still arriving, returns
  // the number of flits received so far.
  uint numFlits() const {
    return data_.size();
  }

  // The number of flits of this packet that have been sent so far. Increment
  // this count using sentFlit().
  uint flitsSent() const {
    return flitsSent_;
  }

  // The number of flits of this packet that have been received so far.
  // Increment this count using receivedFlit().
  uint flitsReceived() const {
    return flitsReceived_;
  }

  // Return the first flit in the packet which hasn't yet been sent. Fails if
  // all flits have already been sent.
  const Flit<T>& getNextFlit() const {
    assert(flitsSent_ < data_.size());
    return data_[flitsSent_];
  }

  // Record that a flit has been sent.
  void sentFlit() {
    assert(flitsSent_ < data_.size());
    flitsSent_++;
  }

  // Record that a flit has been received.
  void addFlit(const Flit<T>& flit) {
    assert(!receivedAllFlits());
    data_.push_back(flit);
    flitsReceived_++;
  }

  // Check whether all flits have been received.
  bool receivedAllFlits() const {
    return data_.back().endOfPacket();
  }

  // Check whether all flits have been sent.
  bool sentAllFlits() const {
    return flitsSent() == numFlits();
  }

protected:

  // Return a particular flit.
  const Flit<T>& getFlit(int index) const {
    return data_[index];
  }

//==============================//
// Local state
//==============================//

private:

  // The contents of the packet.
  vector<Flit<T> > data_;

  // Number of flits sent so far (counted using sentFlit()).
  uint flitsSent_;

  // Number of flits received so far (counted using addFlit()).
  uint flitsReceived_;

};

#endif /* PACKET_H_ */
