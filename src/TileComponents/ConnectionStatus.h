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

  bool isActive() const;
  bool isIdle() const;
  bool isRead() const;
  bool isWrite() const;
  bool isStreaming() const;
  bool readingIPK() const;

  void incrementAddress();
  void clear();
  void teardown();

  int  getChannel() const;
  int  getAddress() const;
  void setChannel(int channel);
  void setReadAddress(int addr);
  void setWriteAddress(int addr);
  void startStreaming();
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

  enum Operation {NONE, LOAD, STORE};

  static const int UNUSED = -1;
  static const int STRIDE = 1;

  int   remoteChannel;
  int   address;
  short operation;
  bool  repeatOperation;

};

#endif /* CONNECTIONSTATUS_H_ */
