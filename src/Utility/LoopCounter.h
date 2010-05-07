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

//==============================//
// Methods
//==============================//

public:

  int value() const;

  int operator+ (int num) const;
  int operator+ (const LoopCounter& ctr) const;
  int operator- (int num) const;
  int operator- (const LoopCounter& ctr) const;

  int operator++ ();
  int operator-- ();
  int operator= (int num);
  int operator+= (int num);
  int operator-= (int num);

  bool operator== (int num) const;
  bool operator!= (int num) const;
  bool operator> (int num) const;
  bool operator>= (int num) const;
  bool operator< (int num) const;
  bool operator<= (int num) const;

private:

  void bringWithinBounds();

//==============================//
// Constructors and destructors
//==============================//

public:

  LoopCounter(int max);
  virtual ~LoopCounter();

//==============================//
// Local state
//==============================//

private:

  // The number of values this counter is allowed to hold (= max value + 1).
  int maximum;

  // The value the counter currently holds.
  int val;

};

#endif /* LOOPCOUNTER_H_ */
