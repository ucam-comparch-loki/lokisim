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
  T *array;    // Use a pre-made Array class instead?

public:

  void put(int position, T& data) {
    if(position>=0 && position<size) array[position] = data;
  }

  T* get(int position) {      // Iterator instead of get method?
    return (position>=0 && position<size) ? &(array[position]) : NULL;
  }

  void merge(Array<T>& other) {
    T* newArray = new T[size + other.size];

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
  Array(int size) {
    this->size = size;
    array = new T[size];

    //for(int i=0; i<size; i++) array[i] = NULL;
  }

  Array() {
    size = 0;
  }

  ~Array() {

  }

/* Necessary functions/operators to pass this datatype down a channel */
  friend void sc_trace(sc_core::sc_trace_file*& tf, const Array<T>& w, std::string& txt) {
    sc_core::sc_trace(tf, w.array, txt);
  }

  bool operator== (const Array<T>& other) {
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
