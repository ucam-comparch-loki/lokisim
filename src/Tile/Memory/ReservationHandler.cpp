/*
 * ReservationHandler.cpp
 *
 *  Created on: 7 May 2015
 *      Author: db434
 */

#include "../../Tile/Memory/ReservationHandler.h"

void ReservationHandler::makeReservation(ComponentID requester, MemoryAddr address) {
  // Determine which reservation slot to write into. First look for a free slot.
  // If there are none, use the slot next to the one used previously.
  for (uint i=0; i<reservations.size(); i++) {
    currentSlot = (currentSlot + 1) % reservations.size();
    if (reservations[currentSlot].valid)
      break;
  }

  reservations[currentSlot].requester = requester;
  reservations[currentSlot].address = address;
  reservations[currentSlot].valid = true;
}

bool ReservationHandler::checkReservation(ComponentID requester, MemoryAddr address) const {
  for (uint i=0; i<reservations.size(); i++) {
    if (reservations[i].requester == requester &&
        reservations[i].address == address &&
        reservations[i].valid)
      return true;
  }

  return false;
}

void ReservationHandler::clearReservation(MemoryAddr address) {
  MemoryAddr word = address & ~0x3; // Mask to invalidate the whole word
  for (uint i=0; i<reservations.size(); i++) {
    if (reservations[i].address == word)
      reservations[i].valid = false;
  }
}

void ReservationHandler::clearReservationRange(MemoryAddr start, MemoryAddr end) {
  for (uint i=0; i<reservations.size(); i++) {
    if ((reservations[i].address >= start) && (reservations[i].address < end))
      reservations[i].valid = false;
  }
}

ReservationHandler::ReservationHandler(uint maxReservations) :
    reservations(maxReservations) {
  currentSlot = 0;
}

ReservationHandler::~ReservationHandler() {
  // Do nothing
}

