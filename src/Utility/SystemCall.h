/*
 * SystemCall.h
 *
 * Used to allow programs to interact with the environment.
 *
 * Lots of this is borrowed from libloki. Should we arrange to have a single
 * shared header for all tools/libraries?
 *
 *  Created on: 25 Apr 2016
 *      Author: db434
 */

#ifndef SRC_UTILITY_SYSTEMCALL_H_
#define SRC_UTILITY_SYSTEMCALL_H_

enum SystemCall {
  SYS_EXIT            = 0x01,   // Terminate the program
  SYS_OPEN            = 0x02,   // Open a file
  SYS_CLOSE           = 0x03,   // Close a file
  SYS_READ            = 0x04,   // Read from a file
  SYS_WRITE           = 0x05,   // Write to a file
  SYS_SEEK            = 0x06,   // Seek within a file

  SYS_TILE_ID         = 0x10,   // (deprecated) Unique ID of this tile. Use \ref get_tile_id instead.
  SYS_POSITION        = 0x11,   // (deprecated) Position within this tile. Use \ref get_core_id instead.

  // These are all lokisim specific.
  SYS_ENERGY_LOG_ON   = 0x20,   // Start recording energy-consuming events
  SYS_ENERGY_LOG_OFF  = 0x21,   // Stop recording energy-consuming events
  SYS_DEBUG_ON        = 0x22,   // Print lots of information to stdout
  SYS_DEBUG_OFF       = 0x23,   // Stop printing debug information
  SYS_INST_TRACE_ON   = 0x24,   // Print address of every instruction executed
  SYS_INST_TRACE_OFF  = 0x25,   // Stop printing instruction addresses
  SYS_SNAPSHOT        = 0x26,   // Print register file contents
  SYS_CLEAR_STATS     = 0x27,   // Reset any statistics collected so far

  SYS_CURRENT_CYCLE   = 0x30    // (deprecated) Get the current cycle number
};



#endif /* SRC_UTILITY_SYSTEMCALL_H_ */
