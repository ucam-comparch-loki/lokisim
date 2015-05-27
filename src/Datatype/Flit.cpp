/*
 * Flit.cpp
 *
 *  Created on: 5 Mar 2014
 *      Author: db434
 */

#include "Flit.h"
#include "Word.h"

// There must be a better way than specialising for every type of Flit I expect
// to use, but for now it's manageable.
template <>
uint Flit<Word>::messageCount = 0;



