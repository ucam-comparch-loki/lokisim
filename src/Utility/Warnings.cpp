/*
 * Warnings.cpp
 *
 *  Created on: 11 Aug 2015
 *      Author: db434
 */

// Warn (rather than terminate) when attempting to modify a read-only memory
// address.
bool WARN_READ_ONLY = true;

// Memory addresses which aren't aligned to a word/half-word boundary are
// automatically rounded down by the memory bank.
bool WARN_UNALIGNED = true;
