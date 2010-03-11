/*
 * MappedStorage.h
 *
 * Represents the class of storage objects where data is accessed using its
 * tag, rather than its position in the structure.
 *
 * Since this class is templated, all of the implementation must go in the
 * header file.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef MAPPEDSTORAGE_H_
#define MAPPEDSTORAGE_H_

#include "Storage.h"

template<class K, class T>
class MappedStorage : public virtual Storage<T> {

public:
  // Returns whether the given address matches any of the tags
  virtual bool checkTags(const K& key) {
    for(unsigned int i=0; i<tags.size(); i++) {
      if(tags.at(i) == key) return true;
    }
    return false;
  }

  // Returns the data corresponding to the given address
  virtual T& read(const K& key) {
    for(unsigned int i=0; i<tags.size(); i++){
      if(tags.at(i) == key) return Storage<T>::data.at(i);
    }
    throw new std::exception();
  }

  // Writes new data to a position determined using the given address
  virtual void write(const K& key, const T& newData) {
    int position = getPosition(key);
    tags.at(position) = key;
    Storage<T>::data.at(position) = newData;
  }

/* Constructors and destructors */
  MappedStorage(int size) : Storage<T>(size), tags(size) {

  }

  virtual ~MappedStorage() {

  }

protected:
/* Local state */
  vector<K> tags;

/* Methods */
  // Returns the position that data with the given address tag should be stored
  virtual int getPosition(const K& key) = 0;

};

#endif /* MAPPEDSTORAGE_H_ */
