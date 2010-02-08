/*
 * Array.h
 *
 * Represents an array of a particular datatype, which can be passed along
 * SystemC channels. This is a more flexible approach than having an array of
 * constant-sized inputs/outputs in each module.
 *
 *  Created on: 22 Jan 2010
 *      Author: db434
 */

#ifndef ARRAY_H_
#define ARRAY_H_

template<class T>
class Array {

  int size;
  std::vector<T> array;    // Use a pre-made Array class instead?

public:

  void put(int position, T& data) {
    if(position>=0 && position<size) array.at(position) = data;
  }

  T& get(int position) const {      // Iterator instead of get method?
    if(position>=0 && position<size) {
      T* result = new T(array.at(position));
      return *result;
    }
    else {
      printf("Asked for position %d when size is %d\n", position, size);
      throw new std::exception();
    }
  }

  void merge(Array<T>& other) {
    std::vector<T> newArray = std::vector<T>(size + other.size);

    for(int i=0; i<size; i++) {
      newArray[i] = array[i];
    }

    for(int i=0; i<other.size; i++) {
      newArray[i+size] = other.array[i];
    }

    size += other.size;
    array = newArray;
  }

/* Constructors and destructors */
  Array(int size) : array(size) {
    this->size = size;
  }

  Array() {
    size = 0;
  }

  ~Array() {

  }

/* Necessary functions/operators to pass this datatype down a channel */
  friend void sc_trace(sc_core::sc_trace_file*& tf, const Array<T>& w, std::string& txt) {
//    for(int i=0; i<w.size; i++) sc_core::sc_trace(tf, w.array.at(i), txt);
  }

  bool operator== (const Array<T>& other) const {
    return (this->array == other.array)    // Probably bad
        && (this->size == other.size);
  }

  Array* operator= (const Array<T>& other) {
    this->array = other.array;
    this->size = other.size;
    return this;
  }

  friend std::ostream& operator<< (std::ostream& os, Array<T> const& v) {

    for(int i=0; i<v.size; i++) {
      os << v.array[i] << " ";
    }

    return os;
  }

};

#endif /* ARRAY_H_ */
