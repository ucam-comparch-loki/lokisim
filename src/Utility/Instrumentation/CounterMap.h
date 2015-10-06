/*
 * CounterMap.h
 *
 * A mapping between elements of a particular type, and integers. It is used
 * to count occurrences of various events.
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef COUNTERMAP_H_
#define COUNTERMAP_H_

#include <map>

template<class T>
class CounterMap {

  typedef unsigned long long count_t;

public:

  typedef typename std::map<T, count_t>::iterator iterator;

  void increment(const T& event, const count_t val = 1) {
    counters[event] += val;
    totalEvents += val;
  }

  const count_t getCount(const T& event) {
    return counters[event];
  }

  const count_t operator[](const T& event) {
    return getCount(event);
  }

  void setCount(const T& event, count_t count) {
    totalEvents += (count - counters[event]);
    counters[event] = count;
  }

  const count_t numEvents() const {
    return totalEvents;
  }

  iterator begin() {
    return counters.begin();
  }

  iterator end() {
    return counters.end();
  }

  void clear() {
    counters.clear();
    totalEvents = 0;
  }

  CounterMap() {
    totalEvents = 0;
  }

private:
  std::map<T, count_t> counters;
  count_t totalEvents;

};

#endif /* COUNTERMAP_H_ */
