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

//==============================//
// Constructors and destructors
//==============================//

public:

  LokiVector2D() {
    size_ = 0;
    data_ = NULL;
  }

  LokiVector2D(size_t width, size_t length) {
    init(width, length);
  }

  virtual ~LokiVector2D() {
    if(data_ != NULL)
      delete[] data_;
  }

//==============================//
// Methods
//==============================//

public:

  // Initialise the vector to the given size. The default constructor will be
  // used to create all contents, so such a constructor must exist.
  inline void init(size_t width, size_t length) {
    init(width);

    for (unsigned int i=0; i<width; i++)
      data_[i].init(length);
  }

  // Initialise with only one parameter - allows different subvectors to be
  // different lengths, if necessary.
  inline void init(size_t width) {
    size_ = width;
    data_ = new LokiVector<T>[width];
  }

  inline const size_t width() const {
    return size_;
  }

  inline const size_t length() const {
    if(data_ != NULL)
      return data_[0].length();
    else
      return 0;
  }

  inline LokiVector<T>& operator[](unsigned int position) const {
    assert(position >= 0);
    assert(position < width());

    return data_[position];
  }

//==============================//
// Local state
//==============================//

private:

  size_t size_;
  LokiVector<T>* data_;

};

#endif /* LOKIVECTOR2D_H_ */
