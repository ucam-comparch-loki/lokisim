/*
 * Grid.h
 *
 * A class representing a two-dimensional array of elements.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 8 Jan 2010
 *      Author: db434
 */

#ifndef GRID_H_
#define GRID_H_

#include <vector>
using std::vector;

template<class T>
class Grid {

//==============================//
// Methods
//==============================//

public:

  // Return the element in the specified position
  const T& get(int row, int column) const {
    if(row>=0 && column>=0 && row<numRows() && column<numColumns()) {
      return *(grid.at(row).at(column));
    }
    else throw std::exception();
  }

  // Replace the element in the specified position by the given element
  void put(int row, int column, T& element) {
    if(row>=0 && column>=0 && row<numRows() && column<numColumns()) {
      grid.at(row).at(column) = &element;
    }
    else throw std::exception();
  }

  // Allows filling of the grid with properly instantiated elements
  void initialPut(int row, T& element) {
    grid.at(row).push_back(&element);
  }

  int numRows() const {
    return rows;
  }

  int numColumns() const {
    return columns;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  Grid(int rows, int columns) : grid(rows) {
    this->rows = rows;
    this->columns = columns;
  }

  virtual ~Grid() {

  }

//==============================//
// Local state
//==============================//

private:

  int                 rows, columns;
  vector<vector<T*> > grid;

};

#endif /* GRID_H_ */
