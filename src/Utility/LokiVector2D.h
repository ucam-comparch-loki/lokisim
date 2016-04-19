/*
 * LokiVector2D.h
 *
 *  Created on: 13 Mar 2012
 *      Author: db434
 */

#ifndef LOKIVECTOR2D_H_
#define LOKIVECTOR2D_H_

#include "LokiVector.h"

template<class T>
class LokiVector2D {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  LokiVector2D() {
    size_ = 0;
    data_ = NULL;
  }

  LokiVector2D(size_t length, size_t width) {
    init(length, width);
  }

  virtual ~LokiVector2D() {
    if (data_ != NULL)
      delete[] data_;
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Initialise the vector to the given size. The default constructor will be
  // used to create all contents, so such a constructor must exist.
  inline void init(size_t length, size_t width) {
    init(length);

    for (unsigned int i=0; i<length; i++)
      data_[i].init(width);
  }

  // Initialise the vector to the same dimensions as another given vector.
  template<typename T2>
  inline void init(const LokiVector2D<T2>& other) {
    init(other.length());

    for (unsigned int i=0; i<length(); i++)
      data_[i].init(other[i]);
  }

  // Initialise with only one parameter - allows different subvectors to be
  // different lengths, if necessary.
  inline void init(size_t length) {
    size_ = length;
    data_ = new LokiVector<T>[length];
  }

  inline const size_t length() const {
    return size_;
  }

  inline LokiVector<T>& operator[](unsigned int position) const {
    assert(length() > 0);
    assert(position < length());

    return data_[position];
  }

  // Call the () operator on each element of the vector. (Useful for binding
  // SystemC ports and signals.)
  template<typename T2>
  inline void operator()(const LokiVector2D<T2>& other) {
    assert(length() == other.length());
    for (uint i=0; i<length(); i++)
      data_[i](other[i]);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  size_t size_;
  LokiVector<T>* data_;

};

#endif /* LOKIVECTOR2D_H_ */
