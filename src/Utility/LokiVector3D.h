/*
 * LokiVector3D.h
 *
 *  Created on: 23 Jan 2015
 *      Author: db434
 */

#ifndef SRC_UTILITY_LOKIVECTOR3D_H_
#define SRC_UTILITY_LOKIVECTOR3D_H_

#include "LokiVector2D.h"

template<class T>
class LokiVector3D {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  LokiVector3D() {
    size_ = 0;
    data_ = NULL;
  }

  LokiVector3D(sc_module_name name, size_t length, size_t width, size_t depth) {
    init(name, length, width, depth);
  }

  virtual ~LokiVector3D() {
    if (data_ != NULL)
      delete[] data_;
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Initialise the vector to the given size. The default constructor will be
  // used to create all contents, so such a constructor must exist.
  inline void init(sc_module_name name, size_t length, size_t width, size_t depth) {
    init(length);

    for (unsigned int i=0; i<length; i++)
      data_[i].init(name, width, depth);
  }

  // Initialise the vector to the same dimensions as another given vector.
  template<typename T2>
  inline void init(sc_module_name name, const LokiVector3D<T2>& other) {
    init(other.size());

    for (unsigned int i=0; i<size(); i++)
      data_[i].init(name, other[i]);
  }

  // Initialise with only one parameter - allows different subvectors to be
  // different lengths, if necessary.
  inline void init(size_t length) {
    assert(length > 0);
    size_ = length;
    data_ = new LokiVector2D<T>[length];
  }

  inline size_t size() const {
    return size_;
  }

  inline LokiVector2D<T>& operator[](unsigned int position) const {
    assert(size() > 0);
    assert(position < size());

    return data_[position];
  }

  // Call the () operator on each element of the vector. (Useful for binding
  // SystemC ports and signals.)
  template<typename T2>
  inline void operator()(const LokiVector3D<T2>& other) {
    assert(size() == other.size());
    for (uint i=0; i<size(); i++)
      data_[i](other[i]);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  size_t size_;
  LokiVector2D<T>* data_;

};




#endif /* SRC_UTILITY_LOKIVECTOR3D_H_ */
