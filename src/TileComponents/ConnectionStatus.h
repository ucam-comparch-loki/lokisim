/*
 * ConnectionStatus.h
 *
 * A data structure used to hold all state required by the MemoryMat's control
 * port. This includes which channel we are connected to, whether reads or
 * writes are taking place, and the address of the operation.
 *
 * Contains lots of very simple methods to abstract away the details.
 *
 *  Created on: 31 Mar 2010
 *      Author: db434
 */

#ifndef CONNECTIONSTATUS_H_
#define CONNECTIONSTATUS_H_

class ConnectionStatus {

//==============================//
// Methods
//==============================//

public:

  // Tells whether there is a connection set up.
  bool active() const;

  // Tells whether there is a connection set up, but not currently carrying
  // out an operation.
  bool idle() const;

  // Tells whether there is a read operation in process at this port.
  bool isRead() const;

  // Tells whether there is a write operation in process at this port.
  bool isWrite() const;

  // Tells whether there is a streaming operation in process (IPK read or
  // STADDR followed by many pieces of data.
  bool streaming() const;

  // Tells whether an instruction packet read is currently taking place.
  bool readingIPK() const;

  // Distinguish between instruction that read/write whole words, and those
  // that deal with individual bytes.
  bool isByteAccess() const;

  // Increments the address of the operation, so the next load/store accesses
  // the next location in memory.
  void incrementAddress();

  // Clears the current operation, but leaves the channel set up.
  void clear();

  // Tears down the channel, making it available for another cluster to
  // connect to.
  void teardown();

  // Return the remote channel connected to this port.
  int  channel() const;

  // Return the address to access in memory.
  int  address() const;

  // Set the remote channel connected to this port.
  void channel(int channel);

  // Set the memory address to read from.
  void readAddress(int addr);

  // Set the memory address to write to.
  void writeAddress(int addr);

  // Start a streaming operation.
  void startStreaming();

  // End a streaming operation.
  void stopStreaming();


//==============================//
// Constructors and destructors
//==============================//

public:

  ConnectionStatus();
  virtual ~ConnectionStatus();

//==============================//
// Local state
//==============================//

private:

  enum Operation {NONE, LOAD, LOADBYTE, STORE, STOREBYTE};

  static const int UNUSED = -1;
  static const int STRIDE = 1;

  static const int BYTES_PER_WORD = 4;

  int   remoteChannel_;
  int   address_;
  short operation_;
  bool  repeatOperation_;

};

#endif /* CONNECTIONSTATUS_H_ */
