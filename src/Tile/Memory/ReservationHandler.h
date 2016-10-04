/*
 * ReservationHandler.h
 *
 * Module used to implement load-linked/store-conditional operations.
 *
 * Load-linked operations make a reservation, and store-conditionals only take
 * effect if the address to which they are storing is still reserved. Any
 * conflicting operation (e.g. overwriting the data) clears the reservation.
 *
 * In the case that more reservations are made than can be handled
 * simultaneously, reservations are overwritten in a roughly-FIFO order.
 *
 *  Created on: 7 May 2015
 *      Author: db434
 */

#ifndef SRC_TILE_MEMORY_RESERVATIONHANDLER_H_
#define SRC_TILE_MEMORY_RESERVATIONHANDLER_H_

#include "../../Datatype/Identifier.h"
#include "../../Tile/Memory/MemoryTypedefs.h"

class ReservationHandler {
public:

  // Make a new reservation at the start of an atomic transaction.
  void makeReservation(ComponentID requester, MemoryAddr address);

  // Check whether the reservation is still valid before committing an atomic
  // transaction.
  bool checkReservation(ComponentID requester, MemoryAddr address) const;

  // Clear all conflicting reservations when their data is invalidated.
  void clearReservation(MemoryAddr address);

  // Clear all reservations in the address range between start (inclusive) and
  // end (exclusive).
  void clearReservationRange(MemoryAddr start, MemoryAddr end);

public:
  ReservationHandler(uint maxReservations);
  virtual ~ReservationHandler();

private:
  struct Reservation {
    ComponentID requester;
    MemoryAddr  address;
    bool        valid;
  };

  std::vector<Reservation> reservations;
  uint currentSlot;
};

#endif /* SRC_TILE_MEMORY_RESERVATIONHANDLER_H_ */
