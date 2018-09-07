/*
 * Identifier.h
 *
 *  Created on: 20 Jan 2015
 *      Author: daniel
 */

#ifndef SRC_IDENTIFIER_H_
#define SRC_IDENTIFIER_H_

#include <assert.h>
#include <systemc>
#include "../Utility/Parameters.h"

#ifndef uint
typedef unsigned int uint;
#endif

struct TileID {
  uint x : 3;
  uint y : 3;

  TileID(uint flattened)       : x((flattened >> 3) & 7), y(flattened & 7) {}
  TileID(uint xPos, uint yPos) : x(xPos), y(yPos) {}
  TileID(const TileID& other)  : x(other.x), y(other.y) {}

  uint flatten()          const {return (x << 3) | y;}

  bool operator==(const TileID other) const {return (x==other.x) && (y==other.y);}
  bool operator!=(const TileID other) const {return !(*this == other);}
  bool operator<(const TileID other) const {return (x<other.x) || (x==other.x && y<other.y);}

  friend std::ostream& operator<< (std::ostream& os, const TileID& t) {
    // Convert a tile address into the form "(column, row)"
    os << "(" << t.x << "," << t.y << ")";
    return os;
  }

  const std::string getNameString() const {
    // Convert a tile address into the form "x_y"
    std::stringstream ss;
    ss << x << "_" << y;
    return ss.str();
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const TileID& w, const std::string& txt) {
    sc_trace(tf, w.x, txt + ".x");
    sc_trace(tf, w.y, txt + ".y");
  }

} __attribute__((packed));


struct ComponentID {
  TileID        tile;
  uint          position : 4;

  ComponentID()                         : tile(0), position(0) {}
  ComponentID(uint flattened)           : tile(flattened >> 4), position(flattened & 0xf) {}
  ComponentID(TileID t, uint pos)       : tile(t), position(pos)   {}
  ComponentID(uint x, uint y, uint pos) : tile(x,y), position(pos) {}
  ComponentID(const ComponentID& other) : tile(other.tile), position(other.position) {}

  uint flatten()  const {return (tile.flatten() << 4) | position;}

  bool operator==(const ComponentID other) const {return (tile==other.tile) && (position==other.position);}
  bool operator!=(const ComponentID other) const {return !(*this == other);}

  friend std::ostream& operator<< (std::ostream& os, const ComponentID& c) {
    // Convert a unique component address into the form "(column, row, position)"
    os << "(" << c.tile.x << "," << c.tile.y << "," << c.position << ")";
    return os;
  }

  const std::string getNameString() const {
    // Convert a unique port address into the form "tileX_tileY_position"
    std::stringstream ss;
    ss << tile.x << "_" << tile.y << "_" << position;
    return ss.str();
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const ComponentID& w, const std::string& txt) {
    sc_trace(tf, w.tile, txt + ".tile");
    sc_trace(tf, w.position, txt + ".position");
  }

} __attribute__((packed));


#define MASK_SIZE 8
struct ChannelID {
  // Would like to union these two, but not allowed.
  ComponentID   component;
  uint          coremask  : MASK_SIZE;

  uint          multicast : 1;
  uint          channel   : 4;

  ChannelID() : component(0,0,0), coremask(0), multicast(0), channel(0) {}
  ChannelID(uint flat) :
    component(flat >> 12),
    coremask((flat >> 5) & 0xff),
    multicast((flat >> 4) & 1),
    channel(flat & 0xf) {}
  ChannelID(uint x, uint y, uint pos, uint ch) :
    component(x,y,pos),
    coremask(0),
    multicast(0),
    channel(ch) {}
  ChannelID(uint mask, uint ch) :
    component(0,0,0),
    coremask(mask),
    multicast(1),
    channel(ch) {}
  ChannelID(ComponentID comp, uint ch) :
    component(comp),
    coremask(0),
    multicast(0),
    channel(ch) {}
  ChannelID(const ChannelID& other) :
    component(other.component),
    coremask(other.coremask),
    multicast(other.multicast),
    channel(other.channel) {}

  uint flatten()  const {return (component.flatten() << 12) | (coremask << 5) | (multicast << 4) | channel;}
  bool isNullMapping() const {return flatten() == 0;}

  ChannelID addPosition(uint position) {
    assert(!multicast && "Can't increment a multicast address");

    uint pos = component.position + position;
    ChannelID id(component.tile.x, component.tile.y, pos, channel);
    assert(id.component.position >= position && "ComponentID.position overflowed");

    return id;
  }

  ChannelID addChannel(uint ch, uint maxChannels) {
    assert(!multicast && "Can't increment a multicast address");

    uint newChannel = channel + ch;
    assert(newChannel < maxChannels);
    ChannelID id(component.tile.x, component.tile.y, component.position, newChannel);
    assert(id.channel >= ch && "ChannelID.channel overflowed");

    return id;
  }

  uint numDestinations() const {
    // Determine how many destination components this message is to be sent to.
    if (multicast)
      return __builtin_popcount(coremask);
    else
      return 1;
  }

  const std::string getString() const {
    // Convert a unique port address into the form "(x, y, position, channel)"

    std::stringstream ss;

    if (multicast) {
      ss << "(m";

      for (uint i=MASK_SIZE-1; (int)i>=0; i--) {
        if ((coremask >> i) & 1) ss << "1";
        else                     ss << "0";
      }

      ss << "," << channel << ")";
    }
    else {
      ss << "(" << component.tile.x << "," << component.tile.y << "," << component.position << "," << channel << ")";
    }

    return ss.str();
  }

  bool operator==(const ChannelID other) const {
    if (multicast)
      return (coremask == other.coremask) && (multicast == other.multicast) && (channel == other.channel);
    else
      return (component == other.component) && (multicast == other.multicast) && (channel == other.channel);
  }

  bool operator!=(const ChannelID other) const {
    return !(*this == other);
  }

  friend std::ostream& operator<< (std::ostream& os, const ChannelID& c) {
    os << c.getString();
    return os;
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const ChannelID& w, const std::string& txt) {
    sc_trace(tf, w.component, txt + ".component");
    sc_trace(tf, w.channel, txt + ".channel");
  }

} __attribute__((packed));


#endif /* SRC_IDENTIFIER_H_ */
