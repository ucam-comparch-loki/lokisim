/*
 * StagingArea.h
 *
 * Component responsible for temporarily holding data and tracking when all
 * elements have been dealt with. It is assumed that each value will only be
 * read or written once.
 *
 *  Created on: 6 Aug 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_STAGINGAREA_H_
#define SRC_TILE_ACCELERATOR_STAGINGAREA_H_

#include <systemc>
#include <vector>

using sc_core::sc_event;
using std::vector;

template <typename T>
class StagingArea {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  StagingArea(size_t width, size_t height) :
      data(height, vector<T>(width, 0)),
      valid(height, vector<bool>(width, false)),
      numValid(0) {

    assert(size() == width * height);
    assert(size() > 0);
  }


//============================================================================//
// Methods
//============================================================================//

public:

  // The number of elements stored.
  size_t size() const {
    return data.size() * data[0].size();
  }

  // Read a value from the given position.
  T read(uint row, uint col) {
    assert(row < data.size());
    assert(col < data[0].size());
    assert(valid[row][col]);

    valid[row][col] = false;
    numValid--;
    if (isEmpty())
      empty.notify(sc_core::SC_ZERO_TIME);

    return data[row][col];
  }

  // Write a value to the given position.
  void write(uint row, uint col, T value) {
    assert(row < data.size());
    assert(col < data[0].size());
    assert(!valid[row][col]);

    valid[row][col] = true;
    numValid++;
    if (isFull())
      full.notify(sc_core::SC_ZERO_TIME);

    data[row][col] = value;
  }

  // Discard all data stored and revert to empty state.
  void discard() {
    for (uint i=0; i<valid.size(); i++)
      valid[i].assign(valid[i].size(), false);
    numValid = 0;
    empty.notify(sc_core::SC_ZERO_TIME);
  }

  // Fill all unused slots with the given value.
  void fillWith(T value) {
    for (uint row=0; row<data.size(); row++)
      for (uint col=0; col<data[row].size(); col++)
        if (!valid[row][col])
          write(row, col, value);
  }

  // Have all values been written since they were last read?
  bool isFull() const {
    return numValid == size();
  }

  // Have all values been read since they were last written?
  bool isEmpty() const {
    return numValid == 0;
  }

  // Event triggered whenever the staging area becomes empty.
  const sc_event& emptiedEvent() const {
    return empty;
  }

  // Event triggered whenever the staging area becomes full.
  const sc_event& filledEvent() const {
    return full;
  }


//============================================================================//
// Local state
//============================================================================//

private:

  vector<vector<T>> data;
  vector<vector<bool>> valid;
  uint numValid;
  sc_event full;
  sc_event empty;

};

#endif /* SRC_TILE_ACCELERATOR_STAGINGAREA_H_ */
