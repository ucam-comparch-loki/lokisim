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

public:

  void increment(const T& event) {
    counters[event] = counters[event] + 1;
    totalEvents++;
  }

  const unsigned long long getCount(const T& event) {
    return counters[event];
  }

  const unsigned long long operator[](const T& event) {
    return getCount(event);
  }

  void setCount(const T& event, unsigned long long count) {
    totalEvents += (count - counters[event]);
    counters[event] = count;
  }

  const unsigned long long numEvents() const {
    return totalEvents;
  }

  CounterMap() {

  }

  virtual ~CounterMap() {

  }

private:
  std::map<T, unsigned long long> counters;
  unsigned long long totalEvents;

};

#endif /* COUNTERMAP_H_ */
