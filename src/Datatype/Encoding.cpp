/*
 * Encoding.cpp
 *
 *  Created on: 4 Feb 2019
 *      Author: db434
 */

#include "Encoding.h"
#include "../Utility/Parameters.h"

TileIDEncoding::TileIDEncoding() {
  // This constructor is not for general use.
  xBits = yBits = xShift = yShift = xMask = yMask = 0;
}

TileIDEncoding::TileIDEncoding(uint xBits, uint yBits) :
    xBits(xBits),
    yBits(yBits),
    xShift(yBits),
    yShift(0),
    xMask((1 << xBits) - 1),
    yMask((1 << yBits) - 1) {

  // Nothing

}

uint TileIDEncoding::totalBits() const {
  return xBits + yBits;
}

ComponentIDEncoding::ComponentIDEncoding() {
  // This constructor is not for general use.
  tile = TileIDEncoding();
  positionBits = tileShift = positionShift = tileMask = positionMask = 0;
}

ComponentIDEncoding::ComponentIDEncoding(const TileIDEncoding& tile, uint positionBits) :
    tile(tile),
    positionBits(positionBits),
    tileShift(positionBits),
    positionShift(0),
    tileMask((1 << tile.totalBits()) - 1),
    positionMask((1 << positionBits) - 1) {

  // Nothing

}

uint ComponentIDEncoding::totalBits() const {
  return tile.totalBits() + positionBits;
}

ChannelIDEncoding::ChannelIDEncoding() {
  // This constructor is not for general use.
  component = ComponentIDEncoding();
  coreMaskBits = mcastFlagBits = channelBits = 0;
  componentShift = coreMaskShift = mcastFlagShift = channelShift = 0;
  componentMask = coreMaskMask = mcastFlagMask = channelMask = 0;
}

ChannelIDEncoding::ChannelIDEncoding(const ComponentIDEncoding& component,
    uint coreMaskBits, uint mcastFlagBits, uint channelBits) :
    component(component),
    coreMaskBits(coreMaskBits),
    mcastFlagBits(mcastFlagBits),
    channelBits(channelBits),
    componentShift(coreMaskBits + mcastFlagBits + channelBits),
    coreMaskShift(mcastFlagBits + channelBits),
    mcastFlagShift(channelBits),
    channelShift(0),
    componentMask((1 << component.totalBits()) - 1),
    coreMaskMask((1 << coreMaskBits) - 1),
    mcastFlagMask((1 << mcastFlagBits) - 1),
    channelMask((1 << channelBits) - 1) {

  // Nothing
  // TODO: could probably make the component and coremask overlap.

}

uint ChannelIDEncoding::totalBits() const {
  return component.totalBits() + coreMaskBits + mcastFlagBits + channelBits;
}

TileIDEncoding      Encoding::softwareTileID;
ComponentIDEncoding Encoding::softwareComponentID;
ChannelIDEncoding   Encoding::softwareChannelID;
TileIDEncoding      Encoding::hardwareTileID;
ComponentIDEncoding Encoding::hardwareComponentID;
ChannelIDEncoding   Encoding::hardwareChannelID;

uint max(uint a, uint b) {return (a>b) ? a : b;}

// Compute the minimum number of bits required to store the given value.
uint bitsRequired(uint value) {
  uint bits = 1;

  while ((1 << (bits-1)) < value)
    bits++;

  return bits;
}

void Encoding::initialise(const chip_parameters_t& params) {
  // Most of the software limitations come from the channel map table
  // https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/architecture/core/channel_map_table/
  // The maximum number of channels comes from the instruction encoding.
  softwareTileID = TileIDEncoding(3, 3);
  softwareComponentID = ComponentIDEncoding(softwareTileID, 4);
  softwareChannelID = ChannelIDEncoding(softwareComponentID, 8, 1, 3);

  // All hardware limitations come from the parameters.
  uint tileXBits = bitsRequired(params.allTiles().width - 1);
  uint tileYBits = bitsRequired(params.allTiles().height - 1);
  uint posBits = bitsRequired(max(params.tile.numCores, params.tile.numMemories) - 1);
  uint coreMaskBits = params.tile.numCores;
  uint channelBits = bitsRequired(max(params.tile.core.numInputChannels,
                                      params.tile.core.numOutputChannels()) - 1);

  hardwareTileID = TileIDEncoding(tileXBits, tileYBits);
  hardwareComponentID = ComponentIDEncoding(hardwareTileID, posBits);
  hardwareChannelID = ChannelIDEncoding(hardwareComponentID, coreMaskBits, 1, channelBits);
}
