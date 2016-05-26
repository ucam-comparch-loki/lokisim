/*
 * LokiVector.h
 *
 * A vector used to contain SystemC modules. An ordinary vector can't be used
 * because it requires that the objects it contains can be copied.
 *
 * This class may end up being very similar to sc_vector, due to be released in
 * SystemC 2.3.
 *
 *  Created on: 13 Mar 2012
 *      Author: db434
 */

#ifndef LOKIVECTOR_H_
#define LOKIVECTOR_H_

#include <assert.h>
#include <cstdlib>

template<class T>
class LokiVector {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  LokiVector() {
    size_ = 0;
    data_ = NULL;
  }

  LokiVector(size_t size) {
    init(size);
  }

  virtual ~LokiVector() {
    if (data_ != NULL)
      delete[] data_;
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Initialise the vector to the given size. The default constructor will be
  // used to create all contents, so such a constructor must exist.
  inline void init(size_t size) {
    assert(size > 0);
    size_ = size;
    data_ = new T[size];
  }

  // Initialise the vector to the same dimensions as another given vector.
  template<typename T2>
  inline void init(const LokiVector<T2>& other) {
    init(other.length());
  }

  inline const size_t length() const {
    return size_;
  }

  inline T& operator[](unsigned int position) const {
    assert(length() > 0);
    assert(position < length());

    return data_[position];
  }

  // Call the () operator on each element of the vector. (Useful for binding
  // SystemC ports and signals.)
  template<typename T2>
  inline void operator()(const LokiVector<T2>& other) {
    assert(length() == other.length());
    for (uint i=0; i<length(); i++)
      data_[i](other[i]);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  size_t size_;
  T* data_;

};

#endif /* LOKIVECTOR_H_ */
