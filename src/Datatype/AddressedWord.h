/*
* AddressedWord.h
*
*  Created on: 5 Jan 2010
*      Author: db434
*/

#ifndef ADDRESSEDWORD_H_
#define ADDRESSEDWORD_H_

#include "../Typedefs.h"
#include "ChannelID.h"
#include "Word.h"

class AddressedWord {
private:
	// The data being transmitted.
	Word payload_;

	// The location the data is being transmitted to.
	ChannelID channelID_;

	// Marks whether or not this is a claim for a port. Once an input port is
	// claimed, all of its responses will be sent back to the sending output port.
	bool portClaim_;

	bool endOfPacket_;
public:
	inline Word payload() const					{return payload_;}
	inline ChannelID channelID() const			{return channelID_;}
	inline bool portClaim() const				{return portClaim_;}
	inline bool endOfPacket() const				{return endOfPacket_;}

	inline void channelID(ChannelID id)			{channelID_ = id;}
	inline void notEndOfPacket()				{endOfPacket_ = false;}

	friend void sc_trace(sc_core::sc_trace_file*& tf, const AddressedWord& w, const std::string& txt) {
		sc_trace(tf, w.payload_, txt + ".payload");
		//sc_core::sc_trace(tf, w.channelID_, txt + ".channelID");
	}

	/* Necessary functions/operators to pass this datatype down a channel */

	inline bool operator== (const AddressedWord& other) const {
		return (this->payload_ == other.payload_)
			&& (this->channelID_ == other.channelID_)
			&& (this->portClaim_ == other.portClaim_)
			&& (this->endOfPacket_ == other.endOfPacket_);
	}

	inline AddressedWord& operator= (const AddressedWord& other) {
		payload_ = other.payload_;
		channelID_ = other.channelID_;
		portClaim_ = other.portClaim_;
		endOfPacket_ = other.endOfPacket_;

		return *this;
	}

	friend std::ostream& operator<< (std::ostream& os, AddressedWord const& v) {
		os << "(" << v.payload_ << " -> " << v.channelID_ << ")";
		return os;
	}

	AddressedWord() {
		payload_ = Word(0);
		channelID_ = ChannelID(0, 0);
	}

	AddressedWord(const Word& w, const ChannelID& id, bool portClaim=false) {
		payload_ = w;
		channelID_ = id;
		portClaim_ = portClaim;
		endOfPacket_ = true;

		/*
		// Add proper check here?

		if((int)id < 0 || (id > TOTAL_INPUT_CHANNELS && id > TOTAL_OUTPUT_CHANNELS)) {
		std::cerr << "Warning: planning to send to channel " << (int)id << std::endl;
		}
		*/
	}
};

#endif /* ADDRESSEDWORD_H_ */
