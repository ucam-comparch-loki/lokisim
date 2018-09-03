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

using sc_core::sc_module_name;

template<class T>
class LokiVector {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  LokiVector() {
    // Nothing
  }

  LokiVector(size_t size, sc_module_name name) {
    init(size, name);
  }

  virtual ~LokiVector() {
    for (uint i=0; i<data.size(); i++)
      delete data[i];
  }

//============================================================================//
// Methods
//============================================================================//

public:

  // Initialise the vector to the given size. The default constructor will be
  // used to create all contents, so such a constructor must exist.
  inline void init(size_t size, sc_module_name name) {
    assert(size > 0);
    for (uint i=0; i<size; i++)
      data.push_back(new T(sc_gen_unique_name(name)));
  }

  // Initialise the vector to the same dimensions as another given vector.
  template<typename T2>
  inline void init(const LokiVector<T2>& other, sc_module_name name) {
    init(other.size(), name);
  }

  inline size_t size() const {
    return data.size();
  }

  inline T& operator[](unsigned int position) const {
    assert(size() > 0);
    assert(position < size());

    return *(data[position]);
  }

  // Call the () operator on each element of the vector. (Useful for binding
  // SystemC ports and signals.)
  template<typename T2>
  inline void operator()(const LokiVector<T2>& other) {
    assert(size() == other.size());
    for (uint i=0; i<size(); i++)
      (*(data[i]))(other[i]);
  }

//============================================================================//
// Local state
//============================================================================//

private:

  std::vector<T*> data;

};

#endif /* LOKIVECTOR_H_ */
