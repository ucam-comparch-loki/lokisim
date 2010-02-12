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

  std::vector<T> array;    // Use a pre-made Array class instead?

public:

  int length() const {
    return array.size();
  }

  void put(int position, const T& data) {
    if(position>=0 && position<length()) array.at(position) = data;
  }

  T& get(int position) const {      // Iterator instead of get method?
    if(position>=0 && position<length()) {
      T* result = new T(array.at(position));
      return *result;
    }
    else {
      printf("Exception in Array.get(%d)\n", position);
      throw new std::exception();
    }
  }

  void merge(const Array<T>& other) {
    std::vector<T> newArray = std::vector<T>(length() + other.length());

    for(int i=0; i<length(); i++) {
      newArray[i] = array[i];
    }

    for(int i=0; i<other.length(); i++) {
      newArray[i+length()] = other.array[i];
    }

    array = newArray;
  }

/* Constructors and destructors */
  Array(int size) : array(size) {

  }

  Array(const Array<T>& other, int start, int end) : array(end-start+1) {

    for(int i=0; i<length(); i++) {
      try {
        array[i] = other.array[start+i];
      }
      catch(std::exception e) {
        printf("Copying from beyond end of Array.");
        array[i] = T();
      }
    }
  }

  Array() : array() {

  }

  ~Array() {

  }

/* Necessary functions/operators to pass this datatype down a channel */
  friend void sc_trace(sc_core::sc_trace_file*& tf, const Array<T>& w, std::string& txt) {
//    for(int i=0; i<w.size; i++) sc_core::sc_trace(tf, w.array.at(i), txt);
  }

  bool operator== (const Array<T>& other) const {
    return (this->array == other.array);   // Probably bad
  }

  Array& operator= (const Array<T>& other) {
    this->array = other.array;
    return *this;
  }

  friend std::ostream& operator<< (std::ostream& os, Array<T> const& v) {

    for(int i=0; i<v.length(); i++) {
      os << v.array[i] << " ";
    }

    return os;
  }

};

#endif /* ARRAY_H_ */
