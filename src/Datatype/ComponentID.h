/*
 * ComponentID.h
 *
 *  Created on: 9 May 2011
 *      Author: afjk2
 */

#ifndef COMPONENTID_H_
#define COMPONENTID_H_

#include "systemc"

#include "../Typedefs.h"
#include "../Utility/Parameters.h"
#include "Word.h"

class ComponentID : public Word {
private:
	static const uint OFFSET_TILE = 20;
	static const uint WIDTH_TILE = 12;
	static const uint OFFSET_POSITION = 12;
	static const uint WIDTH_POSITION = 8;
public:
	inline uint32_t getData() const					{return data_ & 0xFFFFFFFFULL;}

	inline uint getTile() const						{return getBits(OFFSET_TILE, OFFSET_TILE + WIDTH_TILE - 1);}
	inline uint getPosition() const					{return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1);}

	inline uint getColumn() const					{return getTile() % NUM_TILE_COLUMNS;}
	inline uint getRow() const						{return getTile() / NUM_TILE_COLUMNS;}

	inline bool isCore() const						{return !isNullMapping() && getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) < CORES_PER_TILE;}
	inline bool isMemory() const					{return !isNullMapping() && getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) >= CORES_PER_TILE;}

	inline bool isNullMapping() const				{return data_ == 0xFFFFFFFFULL;}

	inline uint getGlobalComponentNumber() const	{return getTile() * COMPONENTS_PER_TILE + getPosition();}
	inline uint getGlobalCoreNumber() const			{return getTile() * CORES_PER_TILE + getPosition();}
	inline uint getGlobalMemoryNumber() const		{return getTile() * MEMS_PER_TILE + getPosition() - CORES_PER_TILE;}

	inline void setData(uint32_t data)				{data_ = data;}

	inline ComponentID addPosition(uint position) {
		uint tile = getTile();
		uint pos = getPosition() + position;
		tile += pos / COMPONENTS_PER_TILE;
		pos %= COMPONENTS_PER_TILE;
		return ComponentID(tile, pos);
	}

	inline const std::string getString() const {
		// Convert a unique port address into the form "(tile, position)"

		std::stringstream ss;
		ss << "(" << getTile() << "," << getPosition() << ")";
		std::string result;
		ss >> result;
		return result;
	}

	friend std::ostream& operator<< (std::ostream& os, const ComponentID& c) {
	  os << c.getString();
    return os;
  }

	inline const std::string getNameString() const {
		// Convert a unique port address into the form "tile_position"

		std::stringstream ss;
		ss << getTile() << "_" << getPosition();
		std::string result;
		ss >> result;
		return result;
	}

	friend void sc_trace(sc_core::sc_trace_file*& tf, const ComponentID& w, const std::string& txt) {
		sc_trace(tf, w.getTile(), txt + ".tile");
		sc_trace(tf, w.getPosition(), txt + ".position");
	}

	ComponentID() : Word(0xFFFFFFFFULL) {
		// Nothing
	}

	ComponentID(uint32_t opaque) : Word(opaque) {
		// Nothing
	}

	ComponentID(const Word& other) : Word(other) {
		// Nothing
	}

	ComponentID(uint tile, uint position) : Word((tile << OFFSET_TILE) | (position << OFFSET_POSITION)) {
		// Nothing
	}
};

#endif /* COMPONENTID_H_ */
