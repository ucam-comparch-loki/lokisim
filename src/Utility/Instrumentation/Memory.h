/*
 * Memory.h
 *
 *  Created on: 16 Jun 2010
 *      Author: db434
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "InstrumentationBase.h"

class Memory : public InstrumentationBase {

public:

  static void read();
  static void write();

  static void printStats();

private:

  static int readCount, writeCount;

};

#endif /* MEMORY_H_ */
