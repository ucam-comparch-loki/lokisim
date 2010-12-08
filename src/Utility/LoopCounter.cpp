/*
 * LoopCounter.cpp
 *
 *  Created on: 6 May 2010
 *      Author: db434
 */

#include "LoopCounter.h"

int LoopCounter::value() const {
  return val;
}

void LoopCounter::setNull() {
  val = -1;
}

bool LoopCounter::isNull() const {
  return (val == -1);
}

int LoopCounter::operator+ (int num) const {
  return (val + num) % maximum;
}

int LoopCounter::operator+ (const LoopCounter& ctr) const {
  return (val + ctr.val) % maximum;
}

int LoopCounter::operator- (int num) const {
  return (val - num + maximum) % maximum;
}

int LoopCounter::operator- (const LoopCounter& ctr) const {
  return (val - ctr.val + maximum) % maximum;
}

int LoopCounter::operator++ () {
  val++;
  bringWithinBounds();
  return val;
}

int LoopCounter::operator-- () {
  val--;
  bringWithinBounds();
  return val;
}

int LoopCounter::operator= (int num) {
  val = num;
  bringWithinBounds();
  return val;
}

int LoopCounter::operator+= (int num) {
  val += num;
  bringWithinBounds();
  return val;
}

int LoopCounter::operator-= (int num) {
  val -= num;
  bringWithinBounds();
  return val;
}

bool LoopCounter::operator== (int num) const {
  return val == num;
}

bool LoopCounter::operator== (LoopCounter& other) const {
  return val == other.val;
}

bool LoopCounter::operator!= (int num) const {
  return val != num;
}

bool LoopCounter::operator> (int num) const {
  return val > num;
}

bool LoopCounter::operator>= (int num) const {
  return val >= num;
}

bool LoopCounter::operator< (int num) const {
  return val < num;
}

bool LoopCounter::operator<= (int num) const {
  return val <= num;
}

void LoopCounter::bringWithinBounds() {
  // Add "maximum" first to ensure we are dealing with a positive number.
  val = (val+maximum) % maximum;
}

LoopCounter::LoopCounter(int max) : maximum(max) {
  val = 0;
}

LoopCounter::~LoopCounter() {

}
