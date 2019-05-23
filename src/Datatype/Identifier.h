/*
 * Identifier.h
 *
 *  Created on: 20 Jan 2015
 *      Author: daniel
 */

#ifndef SRC_IDENTIFIER_H_
#define SRC_IDENTIFIER_H_

#include <assert.h>
#include <sstream>
#include <systemc>
#include "../Utility/Parameters.h"
#include "Encoding.h"

struct TileID {
  uint8_t x;
  uint8_t y;

  TileID(uint flat, TileIDEncoding e) :
      x((flat >> e.xShift) & e.xMask),
      y((flat >> e.yShift) & e.yMask) {}
  TileID(uint xPos, uint yPos) : x(xPos), y(yPos) {}
  TileID(const TileID& other)  : x(other.x), y(other.y) {}

  uint flatten(const TileIDEncoding& e) const {return (x << e.xShift) | (y << e.yShift);}

  bool operator==(const TileID other) const {return (x==other.x) && (y==other.y);}
  bool operator!=(const TileID other) const {return !(*this == other);}
  bool operator<(const TileID other) const {return (x<other.x) || (x==other.x && y<other.y);}

  friend std::ostream& operator<< (std::ostream& os, const TileID& t) {
    // Convert a tile address into the form "(column, row)"
    os << "(" << (uint)t.x << "," << (uint)t.y << ")";
    return os;
  }

  const std::string getNameString() const {
    // Convert a tile address into the form "x_y"
    std::stringstream ss;
    ss << (uint)x << "_" << (uint)y;
    return ss.str();
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const TileID& w, const std::string& txt) {
    sc_trace(tf, w.x, txt + ".x");
    sc_trace(tf, w.y, txt + ".y");
  }

} __attribute__((packed));


struct ComponentID {
  TileID        tile;
  uint8_t       position;

  ComponentID()                         : tile(0,0), position(0) {}
  ComponentID(uint flat, const ComponentIDEncoding& e) :
      tile((flat >> e.tileShift) & e.tileMask, e.tile),
      position((flat >> e.positionShift) & e.positionMask) {}
  ComponentID(TileID t, uint pos)       : tile(t), position(pos)   {}
  ComponentID(uint x, uint y, uint pos) : tile(x,y), position(pos) {}
  ComponentID(const ComponentID& other) : tile(other.tile), position(other.position) {}

  uint flatten(const ComponentIDEncoding& e)  const {
    return (tile.flatten(e.tile) << e.tileShift) | (position << e.positionShift);
  }

  bool operator==(const ComponentID other) const {return (tile==other.tile) && (position==other.position);}
  bool operator!=(const ComponentID other) const {return !(*this == other);}

  friend std::ostream& operator<< (std::ostream& os, const ComponentID& c) {
    // Convert a unique component address into the form "(column, row, position)"
    os << "(" << (uint)c.tile.x << "," << (uint)c.tile.y << "," << (uint)c.position << ")";
    return os;
  }

  const std::string getNameString() const {
    // Convert a unique port address into the form "tileX_tileY_position"
    std::stringstream ss;
    ss << (uint)tile.x << "_" << (uint)tile.y << "_" << (uint)position;
    return ss.str();
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const ComponentID& w, const std::string& txt) {
    sc_trace(tf, w.tile, txt + ".tile");
    sc_trace(tf, w.position, txt + ".position");
  }

} __attribute__((packed));


struct ChannelID {
  // Would like to union these two, but not allowed.
  ComponentID   component;
  uint          coremask;

  bool          multicast;
  uint8_t       channel;

  ChannelID() : component(0,0,0), coremask(0), multicast(0), channel(0) {}
  ChannelID(uint flat, const ChannelIDEncoding& e) :
    component((flat >> e.componentShift) & e.componentMask, e.component),
    coremask((flat >> e.coreMaskShift) & e.coreMaskMask),
    multicast((flat >> e.mcastFlagShift) & e.mcastFlagMask),
    channel((flat >> e.channelShift) & e.channelMask) {}
  ChannelID(TileID tile, uint pos, uint ch) :
    component(tile,pos),
    coremask(0),
    multicast(0),
    channel(ch) {}
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

  uint flatten(const ChannelIDEncoding& e) const {
    return (component.flatten(e.component) << e.componentShift) |
           (coremask << e.coreMaskShift) | (multicast << e.mcastFlagShift) |
           (channel << e.channelShift);
  }

  bool isNullMapping() const {return *this == ChannelID();}

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

  const std::string getString(const ChannelIDEncoding& e) const {
    // Convert a unique port address into the form "(x, y, position, channel)"

    std::stringstream ss;

    if (multicast) {
      ss << "(m";

      for (uint i=e.coreMaskBits-1; (int)i>=0; i--) {
        if ((coremask >> i) & 1) ss << "1";
        else                     ss << "0";
      }

      ss << "," << (uint)channel << ")";
    }
    else {
      ss << "(" << (uint)component.tile.x << "," << (uint)component.tile.y
         << "," << (uint)component.position << "," << (uint)channel << ")";
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
    os << c.getString(Encoding::hardwareChannelID);
    return os;
  }

  friend void sc_trace(sc_core::sc_trace_file*& tf, const ChannelID& w, const std::string& txt) {
    sc_trace(tf, w.component, txt + ".component");
    sc_trace(tf, w.channel, txt + ".channel");
  }

} __attribute__((packed));


#endif /* SRC_IDENTIFIER_H_ */
