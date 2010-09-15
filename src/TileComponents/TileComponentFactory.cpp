/*
 * TileComponentFactory.cpp
 *
 *  Created on: 24 Feb 2010
 *      Author: db434
 */

#include "TileComponentFactory.h"
#include "TileComponent.h"
#include "Cluster.h"
#include "MemoryMat.h"

TileComponent& TileComponentFactory::makeTileComponent(int type, int ID) {

  TileComponent* result;

  switch(type) {
    case(TileComponent::CLUSTER): result = new Cluster("cluster", ID);  break;
    case(TileComponent::MEMORY):  result = new MemoryMat("memory", ID); break;
    default: throw std::exception();
  }

  return *result;

}
