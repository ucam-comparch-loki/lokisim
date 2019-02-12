/*
 * Encoding.h
 *
 * Encodings of various datatypes. Each encoding is given the number of bits
 * occupied by each field, and the position (shift) and bitmask for each field
 * is computed.
 *
 * A field can then be extracted from a value using:
 *     `(value >> fieldShift) & fieldMask`
 *
 *  Created on: 4 Feb 2019
 *      Author: db434
 */

#ifndef SRC_DATATYPE_ENCODING_H_
#define SRC_DATATYPE_ENCODING_H_

#include <cstdint>
#include <cstdlib>
#include "../Utility/Parameters.h"

struct TileIDEncoding {
  uint8_t xBits;
  uint8_t yBits;

  uint8_t xShift;
  uint8_t yShift;

  uint xMask;
  uint yMask;

  TileIDEncoding();
  TileIDEncoding(uint xBits, uint yBits);
  uint totalBits() const;
} __attribute__((packed));


struct ComponentIDEncoding {
  TileIDEncoding tile;
  uint8_t positionBits;

  uint8_t tileShift;
  uint8_t positionShift;

  uint tileMask;
  uint positionMask;

  ComponentIDEncoding();
  ComponentIDEncoding(const TileIDEncoding& tile, uint positionBits);
  uint totalBits() const;
} __attribute__((packed));


struct ChannelIDEncoding {
  ComponentIDEncoding component;
  uint8_t coreMaskBits;
  uint8_t mcastFlagBits;
  uint8_t channelBits;

  uint8_t componentShift;
  uint8_t coreMaskShift;
  uint8_t mcastFlagShift;
  uint8_t channelShift;

  uint componentMask;
  uint coreMaskMask;
  uint mcastFlagMask;
  uint channelMask;

  ChannelIDEncoding();
  ChannelIDEncoding(const ComponentIDEncoding& component, uint coreMaskBits,
                    uint mcastFlagBits, uint channelBits);
  uint totalBits() const;
} __attribute__((packed));


namespace Encoding {
  // Some parts of Loki are limited by encodings exposed to software, but we
  // would like the freedom to explore a wider range of architectures.
  // Maintain separate encodings, one exposed to software, and one used only
  // internally.
  // TODO: it might be possible to avoid having hardware encodings at all, and
  // just transmit un-encoded data.

  // Set up all of the encodings based on the chip's parameters.
  void initialise(const chip_parameters_t& params);

  extern TileIDEncoding softwareTileID;
  extern ComponentIDEncoding softwareComponentID;
  extern ChannelIDEncoding softwareChannelID;

  extern TileIDEncoding hardwareTileID;
  extern ComponentIDEncoding hardwareComponentID;
  extern ChannelIDEncoding hardwareChannelID;
}


#endif /* SRC_DATATYPE_ENCODING_H_ */
