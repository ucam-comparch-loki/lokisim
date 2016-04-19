/*
 * LoopCounter.h
 *
 * A counter which loops back to 0 when it reaches its maximum value.
 *
 *  Created on: 6 May 2010
 *      Author: db434
 */

#ifndef LOOPCOUNTER_H_
#define LOOPCOUNTER_H_

class LoopCounter {

//============================================================================//
// Methods
//============================================================================//

public:

  // Return the value held by this counter.
  int value() const {return val;}

  // Set the counter to a value which isn't allowed in normal circumstances.
  void setNull() {val = -1;}

  // Returns whether the counter has been set to null.
  bool isNull() const {return (val == -1);}

  int operator+ (int num) const {return (val + num) % maximum;}
  int operator- (int num) const {return (val - num + maximum) % maximum;}

  int operator+ (const LoopCounter& ctr) const {return (val + ctr.val) % maximum;}
  int operator- (const LoopCounter& ctr) const {return (val - ctr.val + maximum) % maximum;}

  int operator++ () {
    val++;
    bringWithinBounds();
    return val;
  }

  int operator-- () {
    val--;
    bringWithinBounds();
    return val;
  }

  int operator= (int num) {
    val = num;
    bringWithinBounds();
    return val;
  }

  int operator= (const LoopCounter& other) {
    assert(maximum == other.maximum);
    val = other.val;
    return val;
  }

  int operator+= (int num) {
    val += num;
    bringWithinBounds();
    return val;
  }

  int operator-= (int num) {
    val -= num;
    bringWithinBounds();
    return val;
  }

  bool operator== (int num) const                  {return val == num;}
  bool operator== (const LoopCounter& other) const {return val == other.val;}
  bool operator!= (int num) const                  {return val != num;}
  bool operator>  (int num) const                  {return val >  num;}
  bool operator>= (int num) const                  {return val >= num;}
  bool operator<  (int num) const                  {return val <  num;}
  bool operator<= (int num) const                  {return val <= num;}

private:

  void bringWithinBounds() {
    // Add "maximum" first to ensure we are dealing with a positive number.
    val = (val+maximum) % maximum;
  }

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  LoopCounter(int max) : maximum(max), val(max-1) {
    // Do nothing
  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The number of values this counter is allowed to hold (= max value + 1).
  const int maximum;

  // The value the counter currently holds.
  int val;

};

#endif /* LOOPCOUNTER_H_ */
