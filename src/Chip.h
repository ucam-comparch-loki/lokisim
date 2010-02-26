/*
 * Chip.h
 *
 * A class representing the whole Loki chip as a Grid of Tiles. The Chip is just
 * a means of having all of the Tiles in one place, and does not do anything, as
 * the work is done within the Tiles and their subcomponents.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef CHIP_H_
#define CHIP_H_

#include "Component.h"
#include "Tile.h"
#include "Utility/Grid.h"

class Chip: public Component {

public:
/* Constructors and destructors */
  Chip(sc_core::sc_module_name name, int rows, int columns);
  virtual ~Chip();

private:
/* Components */
  Grid<Tile> tiles;

};

#endif /* CHIP_H_ */
