/*
 * Logging.h
 *
 * Useful macros for logging information.
 *
 *  Created on: 18 Aug 2015
 *      Author: db434
 */

#ifndef SRC_UTILITY_LOGGING_H_
#define SRC_UTILITY_LOGGING_H_

// Log ordinary status information. Toggled on/off using the DEBUG parameter.
#define LOKI_LOG            if (DEBUG) std::cout

// Log curious but non-critical behaviour. Always printed.
#define LOKI_WARN           std::cerr << "Warning: "

// Log critical behaviour. Always printed.
#define LOKI_ERROR          std::cerr << "Error: "

// Useful wrapper for printing hex values.
#define LOKI_HEX(val)       "0x" << std::hex << (val) << std::dec


#endif /* SRC_UTILITY_LOGGING_H_ */
