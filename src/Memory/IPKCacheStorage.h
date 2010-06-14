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
#include "../Datatype/Address.h"
#include "../Datatype/Instruction.h"
#include "../Utility/LoopCounter.h"

class IPKCacheStorage : public MappedStorage<Address, Instruction> {

//==============================//
// Constructors and destructors
//==============================//

public:

  IPKCacheStorage(short size);
  virtual ~IPKCacheStorage();

//==============================//
// Methods
//==============================//

public:

  // Returns whether the given address matches any of the tags
  virtual bool checkTags(const Address& key);

  // Returns the next item in the cache
  virtual Instruction& read();

  // Writes new data to a position determined using the given key
  virtual void write(const Address& key, const Instruction& newData);

  // Jump to a new instruction at a given offset.
  void jump(int offset);

  // Return the memory address of the currently-executing packet.
  Address& packetAddress();

  // Returns the remaining number of entries in the cache.
  int remainingSpace() const;

  // Returns whether the cache is empty. Note that even if a cache is empty,
  // it is still possible to access its contents if an appropriate tag is
  // looked up.
  bool isEmpty() const;

  // Returns whether the cache is full.
  bool isFull() const;

  // Begin reading the packet which is queued up to execute next.
  void switchToPendingPacket();

  void setPersistent(bool persistent);

  // Store some initial instructions in the cache.
  void storeCode(std::vector<Instruction>& code);

private:

  // Returns the data corresponding to the given address.
  // Private because we don't want to use this version for IPK caches.
  virtual Instruction& read(const Address& key);

  // Returns the position that data with the given address tag should be stored
  virtual int getPosition(const Address& key);

  void incrementRefill();
  void incrementCurrent();

  void updateFillCount();

//==============================//
// Local state
//==============================//

private:

  LoopCounter currInst, refill;
  int fillCount;
  int currInstBackup;   // In case it goes NOT_IN_USE and then a jump is used

  bool persistentMode;

  // Do we want a single pending packet, or a queue of them?
  int pendingPacket;  // Location of the next packet to be executed

  // The index of the first instruction of the current instruction packet.
  int currentPacket;

  static const int NOT_IN_USE = -1;

};

#endif /* IPKCACHESTORAGE_H_ */
