/*
 * AcceleratorChannel.h
 *
 * Channel which can communicate a 2D array of values, and associate them with
 * a particular command from the ControlUnit.
 *
 *  Created on: 1 Jul 2019
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_ACCELERATORCHANNEL_H_
#define SRC_TILE_ACCELERATOR_ACCELERATORCHANNEL_H_

#include <vector>

#include "AcceleratorTypes.h"
#include "Interface.h"

using std::vector;

template<typename T>
class AcceleratorChannel : virtual public accelerator_inout_ifc<T> {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  AcceleratorChannel(size2d_t shape) :
    mode(CHANNEL_WRITE),
    shape(shape),
    data(shape.height, vector<T>(shape.width)),
    currentTick(-1) {

    // Nothing

  }

  virtual ~AcceleratorChannel() {}

//============================================================================//
// Methods
//============================================================================//

public:

  // The amount of data that can be transmitted simultaneously.
  virtual size2d_t size() const {
    return shape;
  }

  // Check whether it is possible to write to the channel at this time.
  virtual bool canWrite() const {
    return mode == CHANNEL_WRITE;
  }

  // Event which is triggered when it becomes possible to write to the channel.
  virtual const sc_event& canWriteEvent() const {
    return startWriteMode;
  }

  // Write to the channel, at the given position.
  virtual void write(uint row, uint col, T value) {
    assert(canWrite());
    data[row][col] = value;
  }

  // Notify consumers that all data has now been written. Record which tick of
  // computation this data is associated with.
  virtual void finishedWriting(tick_t tick, sc_time latency = SC_ZERO_TIME) {
    assert(mode == CHANNEL_WRITE);
    mode = CHANNEL_READ;
    currentTick = tick;
    startReadMode.notify(latency);
  }

  // Check whether it is possible to read from the channel at this time.
  virtual bool canRead() const {
    return mode == CHANNEL_READ;
  }

  // Event which is triggered when it becomes possible to read from the channel.
  virtual const sc_event& canReadEvent() const {
    return startReadMode;
  }

  // Return which tick of computation this data is associated with.
  virtual tick_t getTick() const {
    return currentTick;
  }

  // Read from the channel, at the given position. Broadcasting is used to
  // ensure any requested coordinates are valid, even if beyond the bounds of
  // the stored data. A request for row x will access x mod num rows.
  virtual T read(uint row, uint col) const {
    assert(canRead());
    return data[row % shape.height][col % shape.width];
  }

  // Notify producers that all data has now been consumed.
  virtual void finishedReading(sc_time latency = SC_ZERO_TIME) {
    assert(mode == CHANNEL_READ);
    mode = CHANNEL_WRITE;
    startWriteMode.notify(latency);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  // Treat the data as atomic: it must all be read/written before any value
  // can be written/read.
  enum ChannelMode {
    CHANNEL_READ,
    CHANNEL_WRITE
  };

  ChannelMode mode;

  const size2d_t shape;
  vector<vector<T>> data; // Access using data[row][col].
  tick_t currentTick;

  sc_event startReadMode, startWriteMode;

};

#endif /* SRC_TILE_ACCELERATOR_ACCELERATORCHANNEL_H_ */
