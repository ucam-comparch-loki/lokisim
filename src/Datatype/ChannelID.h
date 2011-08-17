/*
 * ChannelID.h
 *
 *  Created on: 28 Apr 2011
 *      Author: afjk2
 */

#ifndef CHANNELID_H_
#define CHANNELID_H_

#include "systemc"

#include "../Typedefs.h"
#include "../Utility/Parameters.h"
#include "ComponentID.h"
#include "Word.h"

class ChannelID : public Word {
private:

  // Layout:
  //
  // 31      27              19              11       7               0
  //  | | | |m|     tile      |    position   |channel|  group + line |
  //
  //  m = multicast flag (for position only - would tile multicast be useful?)

	static const uint OFFSET_CHANNEL = 8;			// Reserve lower 8 bits for group and line bits
	static const uint WIDTH_CHANNEL = 4;
	static const uint OFFSET_POSITION = OFFSET_CHANNEL + WIDTH_CHANNEL;
	static const uint WIDTH_POSITION = 8;
	static const uint OFFSET_TILE = OFFSET_POSITION + WIDTH_POSITION;
	static const uint WIDTH_TILE = 8;
	static const uint OFFSET_MULTICAST = OFFSET_TILE + WIDTH_TILE;
	static const uint WIDTH_MULTICAST = 1;

public:
	inline uint32_t getData() const					{return data_ & 0xFFFFFFFFULL;}

	inline uint getTile() const						{return getBits(OFFSET_TILE, OFFSET_TILE + WIDTH_TILE - 1);}
	inline uint getPosition() const					{return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1);}
	inline uint getChannel() const					{return getBits(OFFSET_CHANNEL, OFFSET_CHANNEL + WIDTH_CHANNEL - 1);}
	inline bool isMulticast() const         {return getBits(OFFSET_MULTICAST, OFFSET_MULTICAST + WIDTH_MULTICAST - 1);}

	inline bool isCore() const						{return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) < CORES_PER_TILE || isMulticast();}
	inline bool isMemory() const					{return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) >= CORES_PER_TILE && !isMulticast();}

	inline bool isNullMapping() const				{return data_ == 0xFFFFFFFFULL;}

	inline void setData(uint32_t data)				{data_ = data;}

	inline uint getGlobalChannelNumber(uint channelsPerTile, uint channelsPerComponent) const {
		return getTile() * channelsPerTile + getPosition() * channelsPerComponent + getChannel();
	}

	inline ChannelID addPosition(uint position) {
		bool multi = isMulticast();
		uint tile = getTile();
		uint pos = getPosition() + position;
		uint ch = getChannel();

		if(!multi) {
      tile += pos / COMPONENTS_PER_TILE;
      pos %= COMPONENTS_PER_TILE;
		}

		return ChannelID(tile, pos, ch, multi);
	}

	inline ChannelID addChannel(uint channel, uint maxChannels) {
    bool multi = isMulticast();
		uint tile = getTile();
		uint pos = getPosition();
		uint ch = getChannel() + channel;

		pos += ch / maxChannels;
		ch %= maxChannels;

		if(!multi) {
      tile += pos / COMPONENTS_PER_TILE;
      pos %= COMPONENTS_PER_TILE;
		}

		return ChannelID(tile, pos, ch, multi);
	}

	inline ComponentID getComponentID() const		{return ComponentID(getTile(), getPosition());}

	inline const std::string getString() const {
		// Convert a unique port address into the form "(tile, position, channel)"

		std::stringstream ss;

		if(isMulticast()) {
		  ss << "(m";
		  unsigned int bitmask = getPosition();

		  for(unsigned int i=CORES_PER_TILE-1; (int)i>=0; i--) {
		    if((bitmask >> i) & 1) ss << "1";
		    else                   ss << "0";
		  }

		  ss << "," << getChannel() << ")";
		}
		else {
		  ss << "(" << getTile() << "," << getPosition() << "," << getChannel() << ")";
		}

		std::string result;
		ss >> result;
		return result;
	}

	friend std::ostream& operator<< (std::ostream& os, const ChannelID& c) {
	  os << c.getString();
    return os;
  }

	friend void sc_trace(sc_core::sc_trace_file*& tf, const ChannelID& w, const std::string& txt) {
		sc_trace(tf, w.getTile(), txt + ".tile");
		sc_trace(tf, w.getPosition(), txt + ".position");
		sc_trace(tf, w.getChannel(), txt + ".channel");
	}

	ChannelID() : Word(0xFFFFFFFFULL) {
		// Nothing
	}

	ChannelID(uint32_t opaque) : Word(opaque) {
		// Nothing
	}

	ChannelID(const Word& other) : Word(other) {
		// Nothing
	}

	ChannelID(uint tile, uint position, uint channel, bool multicast=false) :
	  Word((tile << OFFSET_TILE) | (position << OFFSET_POSITION) | (channel << OFFSET_CHANNEL) | ((multicast?1:0) << OFFSET_MULTICAST)) {
		// Nothing
	}

	ChannelID(const ComponentID& component, uint channel) : Word() {
		data_ = (component.getTile() << OFFSET_TILE) | (component.getPosition() << OFFSET_POSITION) | (channel << OFFSET_CHANNEL);
	}
};

#endif /* CHANNELID_H_ */
