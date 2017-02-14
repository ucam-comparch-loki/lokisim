/*
 * Assert.h
 *
 * Extended assert functions to display more information when there is a
 * problem.
 *
 * These functions can only be called from inside a subclass of Component.
 *
 *  Created on: 10 May 2016
 *      Author: db434
 */

#ifndef SRC_UTILITY_ASSERT_H_
#define SRC_UTILITY_ASSERT_H_

#include <systemc>
#include <string>
#include <iostream>
#include <stdarg.h>

#include "../LokiComponent.h"
#include "../Typedefs.h"
#include "Instrumentation.h"

class Assert {
public:
  static void loki_assert_failure(string test, string function, string file, int line, cycle_count_t cycle, const LokiComponent& component) {
    std::cerr << "Simulation assertion failure" << std::endl
              << "  Cycle:     " << cycle << std::endl
              << "  Component: " << component.name() << std::endl
              << "  Test:      " << test << std::endl
              << "  Function:  " << function << std::endl
              << "  Location:  line " << line << ", " << file << std::endl;
    exit(1);
  }

  static void loki_assert_failure_with_message(string test, string function, string file, int line, cycle_count_t cycle, const LokiComponent& component, string msg, ...) {
    std::cerr << "Simulation assertion failure" << std::endl
              << "  Cycle:     " << cycle << std::endl
              << "  Component: " << component.name() << std::endl
              << "  Test:      " << test << std::endl
              << "  Function:  " << function << std::endl
              << "  Location:  line " << line << ", " << file << std::endl
              << "  Notes:     ";

    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg.c_str(), args);
    va_end(args);

    std::cerr << std::endl;

    exit(1);
  }
};

// An extension of the normal assert() which shows extra information such as
// when and where in the system the problem occurred.
#define loki_assert(test) ((test)               \
    ? __ASSERT_VOID_CAST(0)           \
    : Assert::loki_assert_failure(__STRING(test), __ASSERT_FUNCTION, __FILE__, __LINE__, Instrumentation::currentCycle(), *this))

// Assertion which allows diagnostic information to be displayed if the
// assertion fails. msg is a printf-style format string, and all information
// to be printed is provided afterwards.
#define loki_assert_with_message(test, msg, ...) ((test)               \
    ? __ASSERT_VOID_CAST(0)           \
    : Assert::loki_assert_failure_with_message(__STRING(test), __ASSERT_FUNCTION, __FILE__, __LINE__, Instrumentation::currentCycle(), *this, msg, __VA_ARGS__))


#endif /* SRC_UTILITY_ASSERT_H_ */
