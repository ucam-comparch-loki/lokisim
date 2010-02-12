/*
 * Buffer.h
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include "Storage.h"

template<class T>
class Buffer: public Storage<T> {

/* Local state */
  int readFrom, writeTo, fillCount, size;

/* Methods */
  void incrementReadFrom() {
    readFrom = (readFrom >= size) ? 0 : readFrom+1;
    fillCount--;
  }

  void incrementWriteTo() {
    writeTo = (writeTo >= size) ? 0 : writeTo+1;
    fillCount++;
  }

public:

  virtual T& read() {
    if(!isEmpty()) {
      int i = readFrom;
      incrementReadFrom();
      return Storage<T>::data.at(i);
    }
    else {
//      printf("Exception in Buffer.read()\n");
      throw std::exception();
    }
  }

  virtual void write(const T& newData) {
    if(!isFull()) {
      Storage<T>::data.at(writeTo) = newData;
      incrementWriteTo();
    }
    else {
//      printf("Exception in Buffer.write()\n");
      throw std::exception();
    }
  }

  T& peek() {
    if(!isEmpty()) {
      return (Storage<T>::data.at(readFrom));
    }
    else {
//      printf("Exception in Buffer.peek()\n");
      throw std::exception();
    }
  }

  void discardTop() {
    if(!isEmpty()) incrementReadFrom();
  }

  bool isEmpty() {
    return (fillCount == 0);
  }

  bool isFull() {
    return (fillCount == size);
  }

  void print() {
    for(int i=0; i<this->size; i++) std::cout << Storage<T>::data.at(i) << " ";
    std::cout << std::endl;
  }

/* Constructors and destructors */
  Buffer(int size=16) : Storage<T>(size) {
    readFrom = writeTo = fillCount = 0;
    this->size = size;
  }

  virtual ~Buffer() {

  }

};

#endif /* BUFFER_H_ */
