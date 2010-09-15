/*
 * TileComponentFactory.h
 *
 *  Created on: 24 Feb 2010
 *      Author: db434
 */

#ifndef TILECOMPONENTFACTORY_H_
#define TILECOMPONENTFACTORY_H_

class TileComponent;

class TileComponentFactory {

public:

  // Create a TileComponent of the specified type, and with a certain ID.
  static TileComponent& makeTileComponent(int type, int ID);

};

#endif /* TILECOMPONENTFACTORY_H_ */
