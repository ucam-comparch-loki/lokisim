/*
 * Configuration.cpp
 *
 *  Created on: 16 Aug 2018
 *      Author: db434
 */

#include "Configuration.h"

Configuration::Configuration(size_t peRows, size_t peCols, bool broadcastRows,
                             bool broadcastCols, bool accumulateRows,
                             bool accumulateCols, LoopOrder loops) :
    _peArraySize{peRows, peCols},
    _broadcastRows(broadcastRows),
    _broadcastCols(broadcastCols),
    _accumulateRows(accumulateRows),
    _accumulateCols(accumulateCols),
    _loops(loops) {

  // Nothing.
}

Configuration::Configuration(const Configuration& other) :
    _peArraySize(other._peArraySize),
    _broadcastRows(other._broadcastRows),
    _broadcastCols(other._broadcastCols),
    _accumulateRows(other._accumulateRows),
    _accumulateCols(other._accumulateCols),
    _loops(other._loops) {

  // Nothing.
}

size2d_t Configuration::dma1Ports() const {
  size2d_t size;
  size.height = _peArraySize.height;
  size.width = _broadcastRows ? 1 : _peArraySize.width;

  return size;
}

// Compute the size of the second DMA's port array. This DMA feeds data to
// columns of PEs.
size2d_t Configuration::dma2Ports() const {
  size2d_t size;
  size.width = _peArraySize.width;
  size.height = _broadcastCols ? 1 : _peArraySize.height;

  return size;
}

// Compute the size of the third DMA's port array. This DMA receives computed
// results from the array.
size2d_t Configuration::dma3Ports() const {
  size2d_t size;
  size.height = _accumulateCols ? 1 : _peArraySize.height;
  size.width = _accumulateRows ? 1 : _peArraySize.width;

  return size;
}

size2d_t   Configuration::peArraySize()    const {return _peArraySize;}
bool       Configuration::broadcastRows()  const {return _broadcastRows;}
bool       Configuration::broadcastCols()  const {return _broadcastCols;}
bool       Configuration::accumulateRows() const {return _accumulateRows;}
bool       Configuration::accumulateCols() const {return _accumulateCols;}
const LoopOrder& Configuration::loopOrder() const {return _loops;}
