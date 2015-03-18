/*
* Flit.h
*
* Information to be sent on the network. Includes a payload and a network
* address.
*
* Other out-of-band data is also included for use on some networks.
* TODO: extract this into various specialisations of Flit.
*
*  Created on: 5 Jan 2010
*      Author: db434
*/

#ifndef FLIT_H_
#define FLIT_H_

#include "../Typedefs.h"
#include "Identifier.h"

template <typename T>
class Flit {
private:
  // Give each network message a unique ID. Don't really care about overflow
  // because messages don't last very long.
  static uint     messageCount;
  uint            messageID_;  // Not const because we need to copy it.

  // The data being transmitted.
  T               payload_;

  // The location the data is being transmitted to.
  ChannelID       channelID_;

  // Channel to which data from memory should be returned.
  ChannelIndex    returnAddr_;

  // Marks whether or not this is a claim for a port. Once an input port is
  // claimed, all of its responses will be sent back to the sending output port.
  bool            portClaim_;

  // Indicates whether a connection set up through a port claim message uses
  // credit based flow control.
  // TODO: could this be encoded in the setup message, rather than needing to
  // be sent with every message?
  bool            useCredits_;

  bool            endOfPacket_;

public:
  inline uint           messageID()   const  {return messageID_;}
  inline T              payload()     const  {return payload_;}
  inline ChannelID      channelID()   const  {return channelID_;}
  inline ChannelIndex   returnAddr()  const  {return returnAddr_;}
  inline bool           portClaim()   const  {return portClaim_;}
  inline bool           useCredits()  const  {return useCredits_;}
  inline bool           endOfPacket() const  {return endOfPacket_;}

  inline void setChannelID(const ChannelID& id)        {channelID_ = id;}
  inline void setReturnAddr(const ChannelIndex addr)   {returnAddr_ = addr;}
  inline void setPortClaim(bool portClaim, bool useCredits)  {portClaim_ = portClaim; useCredits_ = useCredits;}
  inline void setEndOfPacket(bool endOfPacket)        {endOfPacket_ = endOfPacket;}

  friend void sc_trace(sc_core::sc_trace_file*& tf, const Flit<T>& w, const std::string& txt) {
    sc_trace(tf, w.payload_, txt + ".payload");
    //sc_core::sc_trace(tf, w.channelID_, txt + ".channelID");
  }

  /* Necessary functions/operators to pass this datatype down a channel */

  inline bool operator== (const Flit<T>& other) const {
    return (this->messageID_    == other.messageID_) // is this line sufficient?
        && (this->payload_      == other.payload_)
        && (this->channelID_    == other.channelID_)
        && (this->returnAddr_   == other.returnAddr_)
        && (this->portClaim_    == other.portClaim_)
        && (this->useCredits_   == other.useCredits_)
        && (this->endOfPacket_  == other.endOfPacket_);
  }

  inline Flit<T>& operator= (const Flit<T>& other) {
    messageID_    = other.messageID_;
    payload_      = other.payload_;
    channelID_    = other.channelID_;
    returnAddr_   = other.returnAddr_;
    portClaim_    = other.portClaim_;
    useCredits_   = other.useCredits_;
    endOfPacket_  = other.endOfPacket_;

    return *this;
  }

  friend std::ostream& operator<< (std::ostream& os, Flit<T> const& v) {
    os << "[" << v.payload_ << " -> " << v.channelID_.getString() << " " << v.portClaim_ << "|" << v.useCredits_ << "|" << v.endOfPacket_ << "] (id:" << v.messageID_ << ")";
    return os;
  }

  Flit<T>() :
    messageID_(messageCount++),
    payload_(static_cast<T>(0)),
    channelID_(ChannelID(0, 0, 0, 0)),
    returnAddr_(0),
    portClaim_(false),
    useCredits_(true),
    endOfPacket_(true) {
  }

  Flit<T>(const T& w, const ChannelID& id) :
    messageID_(messageCount++),
    payload_(w),
    channelID_(id),
    returnAddr_(0),
    portClaim_(false),
    useCredits_(true),
    endOfPacket_(true) {
    /*
    // Add proper check here?

    if((int)id < 0 || (id > TOTAL_INPUT_CHANNELS && id > TOTAL_OUTPUT_CHANNELS)) {
    std::cerr << "Warning: planning to send to channel " << (int)id << std::endl;
    }
    */
  }

};

#endif /* FLIT_H_ */
