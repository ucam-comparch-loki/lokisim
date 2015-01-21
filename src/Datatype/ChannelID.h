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
  // Point-to-point
  // 31    14           8       4 3     0
  //  | ... |    tile   |  pos  |0| ch  |
  //
  // Multicast
  // 31        12               4 3     0
  //  |   ...   |    bitmask    |1| ch  |

	static const uint WIDTH_CHANNEL     = 3;
	static const uint WIDTH_MULTICAST   = 1;
	static const uint WIDTH_POSITION    = 4;
  static const uint WIDTH_TILE_X      = 3;
  static const uint WIDTH_TILE_Y      = 3;
  static const uint WIDTH_TILE        = WIDTH_TILE_X      + WIDTH_TILE_Y;
	static const uint WIDTH_COREMASK    = 8;

	static const uint OFFSET_CHANNEL    = 0;
	static const uint OFFSET_MULTICAST  = OFFSET_CHANNEL    + WIDTH_CHANNEL;
	static const uint OFFSET_POSITION   = OFFSET_MULTICAST  + WIDTH_MULTICAST;
	static const uint OFFSET_TILE       = OFFSET_POSITION   + WIDTH_POSITION;
	static const uint OFFSET_TILE_Y     = OFFSET_TILE;
	static const uint OFFSET_TILE_X     = OFFSET_TILE_Y     + WIDTH_TILE_Y;
	static const uint OFFSET_COREMASK   = OFFSET_POSITION;

public:

	inline uint getTileColumn() const {return getBits(OFFSET_TILE_X, OFFSET_TILE_X + WIDTH_TILE_X - 1);}
	inline uint getTileRow()    const {return getBits(OFFSET_TILE_Y, OFFSET_TILE_Y + WIDTH_TILE_Y - 1);}
	inline uint getPosition()   const {return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1);}
	inline uint getChannel()    const {return getBits(OFFSET_CHANNEL, OFFSET_CHANNEL + WIDTH_CHANNEL - 1);}
	inline bool isMulticast()   const {return getBits(OFFSET_MULTICAST, OFFSET_MULTICAST + WIDTH_MULTICAST - 1);}
	inline uint getCoreMask()   const {return getBits(OFFSET_COREMASK, OFFSET_COREMASK + WIDTH_COREMASK - 1);}

	inline bool isCore()        const {return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) < CORES_PER_TILE || isMulticast();}
	inline bool isMemory()      const {return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) >= CORES_PER_TILE && !isMulticast();}
	inline bool isNullMapping() const {return data_ == 0xFFFFFFFFULL;}

	// The tile number in a one-dimensional space (rather than 2D grid layout).
	// Includes only compute tiles (no I/O tiles) - useful for vectors of ports
	// and signals.
	// Value increases from left-to-right, top-to-bottom.
  inline uint getComputeTile() const {return ((getTileRow() - 1) * COMPUTE_TILE_COLUMNS) + (getTileColumn() - 1);}

	inline uint getGlobalChannelNumber(uint channelsPerTile, uint channelsPerComponent) const {
		return getComputeTile() * channelsPerTile + getPosition() * channelsPerComponent + getChannel();
	}

	inline ChannelID addPosition(uint position) {
	  if (isMulticast()) {
	    assert(false && "Can't increment a multicast address");
	    return ChannelID();
	  }
	  else {
      uint pos = getPosition() + position;

      assert(pos < COMPONENTS_PER_TILE);

      return ChannelID(getTileColumn(), getTileRow(), pos, getChannel());
	  }
	}

	inline ChannelID addChannel(uint channel, uint maxChannels) {
    if (isMulticast()) {
      assert(false && "Can't increment a multicast address");
      return ChannelID();
    }
    else {
      uint ch = getChannel() + channel;

      assert(ch < maxChannels);

      return ChannelID(getTileColumn(), getTileRow(), getPosition(), ch);
    }
	}

	inline int numDestinations() const {
	  // Determine how many destination components this message is to be sent to.
	  if (isMulticast())
	    return __builtin_popcount(getCoreMask());
	  else
	    return 1;
	}

	inline ComponentID getComponentID() const		{return ComponentID(getTileColumn(), getTileRow(), getPosition());}

	inline const std::string getString() const {
		// Convert a unique port address into the form "(tile, position, channel)"

		std::stringstream ss;

		if (isMulticast()) {
		  ss << "(m";
		  unsigned int bitmask = getCoreMask();

		  for (unsigned int i=CORES_PER_TILE-1; (int)i>=0; i--) {
		    if ((bitmask >> i) & 1) ss << "1";
		    else                    ss << "0";
		  }

		  ss << "," << getChannel() << ")";
		}
		else {
		  ss << "(" << getComputeTile() << "," << getPosition() << "," << getChannel() << ")";
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
		sc_trace(tf, w.getComputeTile(), txt + ".tile");
		sc_trace(tf, w.getPosition(), txt + ".position");
		sc_trace(tf, w.getChannel(), txt + ".channel");
	}

	ChannelID() : Word(0xFFFFFFFFULL) {
		// Nothing
	}

	ChannelID(uint32_t opaque) : Word(opaque) {
		// Nothing
	}

  // For point-to-point communication.
  ChannelID(uint tileCol, uint tileRow, uint position, uint channel) :
    Word((tileCol << OFFSET_TILE_X) | (tileRow << OFFSET_TILE_Y) | (position << OFFSET_POSITION) | (channel << OFFSET_CHANNEL)) {

    if ((tileCol >= TOTAL_TILE_COLUMNS) || (tileRow >= TOTAL_TILE_ROWS)) {
      std::cerr << "Creating ChannelID with tile=" << tileCol << "," << tileRow << "; max is " << (TOTAL_TILE_COLUMNS-1) << "," << (TOTAL_TILE_ROWS-1) << std::endl;
      assert(false);
    }

    if (position >= COMPONENTS_PER_TILE)
      std::cerr << "Creating ChannelID with position=" << position << "; max is " << COMPONENTS_PER_TILE << std::endl;
    assert(position < COMPONENTS_PER_TILE);

    if (!(channel < CORE_INPUT_CHANNELS || channel < MEMORY_INPUT_CHANNELS))
      std::cerr << "Creating ChannelID with channel=" << channel << "; max is " << MEMORY_INPUT_CHANNELS << std::endl;
    assert(channel < CORE_INPUT_CHANNELS || channel < MEMORY_INPUT_CHANNELS);

  }

	// For multicast communication.
	ChannelID(uint coremask, uint channel) :
	  Word((coremask << OFFSET_COREMASK) | (channel << OFFSET_CHANNEL) | (1 << OFFSET_MULTICAST)) {

    // FIXME: should multicasting to no one actually be an error?
	  assert(coremask != 0 && "Multicasting to no one");

    if (!(channel < CORE_INPUT_CHANNELS || channel < MEMORY_INPUT_CHANNELS))
      std::cerr << "Creating ChannelID with channel=" << channel << "; max is " << MEMORY_INPUT_CHANNELS << std::endl;
    assert(channel < CORE_INPUT_CHANNELS || channel < MEMORY_INPUT_CHANNELS);

	}

	ChannelID(const ComponentID component, uint channel) : Word() {
		data_ = (component.getTileColumn() << OFFSET_TILE_X) | (component.getTileRow() << OFFSET_TILE_Y) | (component.getPosition() << OFFSET_POSITION) | (channel << OFFSET_CHANNEL);
	}
};

#endif /* CHANNELID_H_ */
