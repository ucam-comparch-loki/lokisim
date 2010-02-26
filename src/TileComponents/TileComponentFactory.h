/*
 * TileComponentFactory.h
 *
 *  Created on: 24 Feb 2010
 *      Author: db434
 */

#ifndef TILECOMPONENTFACTORY_H_
#define TILECOMPONENTFACTORY_H_

#include "TileComponent.h"

class TileComponentFactory {

public:
  static TileComponent& makeTileComponent(int type, int ID);

};

#endif /* TILECOMPONENTFACTORY_H_ */
