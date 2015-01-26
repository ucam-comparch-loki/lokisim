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

//==============================//
// Constructors and destructors
//==============================//

public:

  LokiVector3D() {
    size_ = 0;
    data_ = NULL;
  }

  LokiVector3D(size_t length, size_t width, size_t depth) {
    init(length, width, depth);
  }

  virtual ~LokiVector3D() {
    if (data_ != NULL)
      delete[] data_;
  }

//==============================//
// Methods
//==============================//

public:

  // Initialise the vector to the given size. The default constructor will be
  // used to create all contents, so such a constructor must exist.
  inline void init(size_t length, size_t width, size_t depth) {
    init(length);

    for (unsigned int i=0; i<length; i++)
      data_[i].init(width, depth);
  }

  // Initialise with only one parameter - allows different subvectors to be
  // different lengths, if necessary.
  inline void init(size_t length) {
    size_ = length;
    data_ = new LokiVector2D<T>[length];
  }

  inline const size_t length() const {
    return size_;
  }

  inline LokiVector2D<T>& operator[](unsigned int position) const {
    assert(length() > 0);
    assert(position < length());

    return data_[position];
  }

//==============================//
// Local state
//==============================//

private:

  size_t size_;
  LokiVector2D<T>* data_;

};




#endif /* SRC_UTILITY_LOKIVECTOR3D_H_ */
