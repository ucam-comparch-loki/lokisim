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

  int rows, columns;
  vector<vector<T> > *grid;

public:

  T* get(int row, int column) const {
    if(row>=0 && column>=0 && row<numRows() && column<numColumns()) {
      T* result = &(grid->at(row).at(column));      // This could be wrong
      return result;
    }
    else throw new std::exception();
  }

  void put(int row, int column, T& element) {
    if(row>=0 && column>=0 && row<numRows() && column<numColumns()) {
      grid->at(row).at(column) = element;       // This could be wrong
    }
    else throw new std::exception();
  }

  int numRows() const {
    return rows;
  }

  int numColumns() const {
    return columns;
  }

  Grid(int rows, int columns) {
    grid = new vector<vector<T> >(rows, vector<T>(columns));
    this->rows = rows;
    this->columns = columns;
  }

  virtual ~Grid() {

  }

  // Iterators?
};

#endif /* GRID_H_ */
