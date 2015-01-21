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

  static const uint WIDTH_POSITION  = 8;
  static const uint WIDTH_TILE_Y    = 3;
  static const uint WIDTH_TILE_X    = 3;
  static const uint WIDTH_TILE      = WIDTH_TILE_X    + WIDTH_TILE_Y;

  static const uint OFFSET_POSITION = 0;
  static const uint OFFSET_TILE     = OFFSET_POSITION + WIDTH_POSITION;
  static const uint OFFSET_TILE_Y   = OFFSET_TILE;
  static const uint OFFSET_TILE_X   = OFFSET_TILE_Y   + WIDTH_TILE_Y;

public:

  inline uint getTileColumn()             const {return getBits(OFFSET_TILE_X, OFFSET_TILE_X + WIDTH_TILE_X - 1);}
  inline uint getTileRow()                const {return getBits(OFFSET_TILE_Y, OFFSET_TILE_Y + WIDTH_TILE_Y - 1);}
  inline uint getPosition()               const {return getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1);}

  inline bool isCore()                    const {return !isNullMapping() && getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) < CORES_PER_TILE;}
  inline bool isMemory()                  const {return !isNullMapping() && getBits(OFFSET_POSITION, OFFSET_POSITION + WIDTH_POSITION - 1) >= CORES_PER_TILE;}
  inline bool isNullMapping()             const {return data_ == 0xFFFFFFFFULL;}

  inline uint getComputeTile()            const {return ((getTileRow() - 1) * COMPUTE_TILE_COLUMNS) + (getTileColumn() - 1);}
  inline uint getGlobalComponentNumber()  const {return getComputeTile() * COMPONENTS_PER_TILE + getPosition();}
  inline uint getGlobalCoreNumber()       const {return getComputeTile() * CORES_PER_TILE + getPosition();}
  inline uint getGlobalMemoryNumber()     const {return getComputeTile() * MEMS_PER_TILE + getPosition() - CORES_PER_TILE;}

  inline const std::string getString() const {
    // Convert a unique component address into the form "(column, row, position)"

    std::stringstream ss;
    ss << "(" << getTileColumn() << "," << getTileRow() << "," << getPosition() << ")";
    std::string result;
    ss >> result;
    return result;
  }

  friend std::ostream& operator<< (std::ostream& os, const ComponentID& c) {
    os << c.getString();
    return os;
  }

  inline const std::string getNameString() const {
    // Convert a unique port address into the form "tileX,tileY_position"

    std::stringstream ss;
    ss << getTileColumn() << "," << getTileRow() << "_" << getPosition();
    std::string result;
    ss >> result;
    return result;
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const ComponentID& w, const std::string& txt) {
    sc_trace(tf, w.getComputeTile(), txt + ".tile");
    sc_trace(tf, w.getPosition(), txt + ".position");
  }

  ComponentID() : Word(0xFFFFFFFFULL) {
    // Nothing
  }

  ComponentID(uint opaque) : Word(opaque) {
    // Nothing
  }

  ComponentID(uint tileX, uint tileY, uint position) :
    Word((tileX << OFFSET_TILE_X) | (tileY << OFFSET_TILE_Y) | (position << OFFSET_POSITION)) {
    // Nothing
  }
};

#endif /* COMPONENTID_H_ */
