/*
 * Configuration.h
 *
 * All parameters required to instantiate an accelerator.
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_CONFIGURATION_H_
#define SRC_TILE_ACCELERATOR_CONFIGURATION_H_

#include "AcceleratorTypes.h"
#include "Loops.h"

class Configuration {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Configuration(size_t peRows, size_t peCols, bool broadcastRows,
                bool broadcastCols, bool accumulateRows, bool accumulateCols,
                LoopOrder loops);


//============================================================================//
// Methods
//============================================================================//

public:

  // Size of the PE array.
  size2d_t peArraySize() const;

  // Compute the size of the first DMA's port array. This DMA feeds data to rows
  // of PEs.
  size2d_t dma1Ports() const;

  // Compute the size of the second DMA's port array. This DMA feeds data to
  // columns of PEs.
  size2d_t dma2Ports() const;

  // Compute the size of the third DMA's port array. This DMA receives computed
  // results from the array.
  size2d_t dma3Ports() const;

  bool broadcastRows() const;
  bool broadcastCols() const;
  bool accumulateRows() const;
  bool accumulateCols() const;

  const LoopOrder& loopOrder() const;

//============================================================================//
// Local state
//============================================================================//

private:

  // Size of PE array.
  const size2d_t _peArraySize;

  // Connectivity within PE array.
  // Alternatively, rowConnectivity = {NONE, SYSTOLIC, BROADCAST}.
  const bool _broadcastRows;
  const bool _broadcastCols;
  const bool _accumulateRows;
  const bool _accumulateCols;

  // DMA configuration.
  // Nothing yet - perhaps local caches eventually.

  // Computation details.
  const LoopOrder _loops;

};

#endif /* SRC_TILE_ACCELERATOR_CONFIGURATION_H_ */
