/*
 * IPKCacheStorage.h
 *
 * A special version of the MappedStorage class with behaviour tailored to
 * the Instruction Packet Cache.
 *
 *  Created on: 26 Jan 2010
 *      Author: db434
 */

#ifndef IPKCACHESTORAGE_H_
#define IPKCACHESTORAGE_H_

#include "MappedStorage.h"
#include "../Utility/LoopCounter.h"
#include "../Datatype/Address.h"

class Instruction;

class IPKCacheStorage : public MappedStorage<Address, Instruction> {

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheStorage(const uint16_t size, std::string name);
  virtual ~IPKCacheStorage();

//==============================//
// Methods
//==============================//

public:

  // Returns whether the given address matches any of the tags.
  virtual bool checkTags(const Address& key);

  // Returns the next item in the cache.
  virtual Instruction& read();

  // Writes new data to a position determined using the given key.
  virtual void write(const Address& key, const Instruction& newData);

  // Jump to a new instruction at a given offset.
  void jump(const int16_t offset);

  // Return the memory address of the currently-executing packet.
  Address packetAddress() const;

  // Returns the remaining number of entries in the cache.
  uint16_t remainingSpace() const;

  // Return the memory index of the instruction sent most recently.
  Address getInstLocation() const;

  // Returns whether the cache is empty. Note that even if a cache is empty,
  // it is still possible to access its contents if an appropriate tag is
  // looked up.
  bool isEmpty() const;

  // Returns whether the cache is full.
  bool isFull() const;

  // Begin reading the packet which is queued up to execute next.
  void switchToPendingPacket();

  void setPersistent(const bool persistent);

  // Store some initial instructions in the cache.
  void storeCode(const std::vector<Instruction>& code);

private:

  // Returns the data corresponding to the given address.
  // Private because we don't want to use this version for IPK caches.
  virtual Instruction& read(const Address& key);

  // Returns the position that data with the given address tag should be stored
  virtual uint16_t getPosition(const Address& key);

  void incrementRefill();
  void incrementCurrent();

  void updateFillCount();

//==============================//
// Local state
//==============================//

private:

  // Current instruction pointer and refill pointer.
  LoopCounter currInst, refill;

  uint16_t fillCount;
  uint16_t currInstBackup;   // In case it goes NOT_IN_USE and then a jump is used

  // Tells if we are reading the same packet repeatedly
  bool persistentMode;

  // Location of the next packet to be executed.
  // Do we want a single pending packet, or a queue of them?
  uint16_t pendingPacket;

  // The index of the first instruction of the current instruction packet.
  uint16_t currentPacket;

  // Knowing what the last operation was helps us determine whether the cache
  // is full or empty when the current instruction and refill pointer are in
  // the same place. If we read an instruction, the cache can be considered
  // empty, but if we wrote one, it is full.
  bool lastOpWasARead;

  // For debug purposes, we store the memory address which each instruction is
  // from. This allows us to set breakpoints easily, and also allows us to
  // determine which parts of the program are the hotspots.
  vector<Address> locations;

  // Store the most recent instruction address, so it can be accessed easily.
  Address previousLocation;

  static const uint16_t NOT_IN_USE = -1;

};

#endif /* IPKCACHESTORAGE_H_ */
