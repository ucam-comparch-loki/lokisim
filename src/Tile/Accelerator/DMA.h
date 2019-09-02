/*
 * DMA.h
 *
 * Units responsible for autonomously fetching or storing blocks of data.
 *
 *  Created on: 17 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_DMA_H_
#define SRC_TILE_ACCELERATOR_DMA_H_

#include "../../Exceptions/InvalidOptionException.h"
#include "../../LokiComponent.h"
#include "../../Network/Interface.h"
#include "../../Utility/Assert.h"
#include "AcceleratorTypes.h"
#include "CommandQueue.h"
#include "ConvolutionAlgorithm.h"
#include "MemoryInterface.h"
#include "StagingArea.h"

class Accelerator;

// Base class for all DMA components.
class DMA: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Command from the control unit.
  CommandInput iCommand;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMA);

  DMA(sc_module_name name, ComponentID id, size_t queueLength=4);


//============================================================================//
// Methods
//============================================================================//

public:

  void enqueueCommand(const dma_command_t command);

  bool canAcceptCommand() const;

  void replaceMemoryMapping(EncodedCMTEntry mapEncoded);

  // Magic connection from memory.
  void deliverDataInternal(const NetworkData& flit);

protected:

  // Remove and return the next command from the control unit.
  const dma_command_t dequeueCommand();

  // Generate a new memory request and send it to the appropriate memory bank.
  void createNewRequest(position_t position, MemoryAddr address,
                        MemoryOpcode op, int data = 0);

  // Send a request to memory. If a response is expected, it will trigger the
  // memoryInterface.responseArrivedEvent() event.
  void memoryAccess(position_t position, MemoryAddr address, MemoryOpcode op,
                    int data = 0);

  // Instantly send a request to memory. This is not a substitute for
  // memoryAccess above: this method is only to be called once all details about
  // the access have been confirmed.
  void magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address,
                         ChannelID returnChannel, Word data = 0);

  // These methods need to be overridden because their implementation depends
  // on the type of data being accessed.
  virtual void loadData(position_t position, MemoryAddr address) = 0;
  virtual void storeData(position_t position, MemoryAddr address, int data) = 0;
  virtual void loadAndAdd(position_t position, MemoryAddr address, int data) = 0;

  Accelerator* parent() const;

private:

  void newCommandArrived();


//============================================================================//
// Local state
//============================================================================//

protected:

  // Commands from control unit telling which data to load/store.
  CommandQueue commandQueue;

  // Component responsible for organising requests and responses from memory.
  friend class MemoryInterface;
  MemoryInterface memoryInterface;

};


// Secondary base class which contains type-dependent code.
template <typename T>
class DMABase: public DMA {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // Also include cache details?
  DMABase(sc_module_name name, ComponentID id, size2d_t ports,
          size_t queueLength=4) :
      DMA(name, id, queueLength),
      stagingArea(ports.width, ports.height) {

    // Nothing.

  }


//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void loadData(position_t position, MemoryAddr address) {
    MemoryOpcode op;

    // The control unit doesn't know/care which type of data to access, so
    // convert a general "load" request to a load of the correct size.
    switch (sizeof(T)) {
      case 1:
        op = LOAD_B;
        break;
      case 2:
        op = LOAD_HW;
        break;
      case 4:
        op = LOAD_W;
        break;
      default:
        throw InvalidOptionException("Convolution datatype size", sizeof(T));
        break;
    }

    // Generate a memory request and send to memory.
    createNewRequest(position, address, op);
  }

  virtual void storeData(position_t position, MemoryAddr address, int data) {
    MemoryOpcode op;

    // The control unit doesn't know/care which type of data to access, so
    // convert a general "store" request to a load of the correct size.
    switch (sizeof(T)) {
      case 1:
        op = STORE_B;
        break;
      case 2:
        op = STORE_HW;
        break;
      case 4:
        op = STORE_W;
        break;
      default:
        throw InvalidOptionException("Convolution datatype size", sizeof(T));
        break;
    }

    // Generate a memory request and send to memory.
    createNewRequest(position, address, op, data);
  }

  virtual void loadAndAdd(position_t position, MemoryAddr address, int data) {
    int shiftedData;

    // Memory only supports load-and-adds of whole words. We can emulate the
    // operation on smaller datatypes by shifting the data to the appropriate
    // position. This only works if there is no overflow into neighbouring
    // values.
    // Note also that only integer types are supported.
    switch (sizeof(T)) {
      case 1:
        data &= 0xFF;
        shiftedData = data << ((address & 3) * 8);
        break;
      case 2:
        data &= 0xFFFF;
        shiftedData = data << ((address & 1) * 16);
        break;
      case 4:
        shiftedData = data;
        break;
      default:
        throw InvalidOptionException("Convolution datatype size", sizeof(T));
        break;
    }

    // Generate a memory request and send to memory.
    createNewRequest(position, address, LOAD_AND_ADD, shiftedData);
  }


//============================================================================//
// Local state
//============================================================================//

protected:

  // A register for each port to the compute units. Used for holding data
  // immediately before sending/receiving it to/from the compute unit.
  StagingArea<T> stagingArea;

};


template <typename T>
class DMAInput: public DMABase<T> {

//============================================================================//
// Ports
//============================================================================//

public:

  // Array of values sent to PEs. Addressed using array[x][y].
  LokiVector2D<sc_out<T>> oDataToPEs;

  // Valid signal for data sent to PEs.
  sc_out<bool>            oDataValid;

  // Flow control from PEs. Single signal telling that all are ready.
  sc_in<bool>             iReadyForData;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMAInput);

  DMAInput(sc_module_name name, ComponentID id, size2d_t ports,
           size_t queueLength=4) :
      DMABase<T>(name, id, ports, queueLength),
      oDataToPEs(ports.width, ports.height, "oDataToPEs"),
      oDataValid("oDataValid"),
      iReadyForData("iReadyForData") {

    // Templated class means `this` must be used whenever referring to anything
    // from a parent class.

    SC_METHOD(executeCommand);
    this->sensitive << this->commandQueue.queueChangedEvent();
    this->dont_initialize();

    SC_METHOD(receiveMemoryData);
    this->sensitive << this->memoryInterface.responseArrivedEvent();
    this->dont_initialize();

    SC_METHOD(sendPEData);
    this->sensitive << iReadyForData.pos();
    // do initialise

  }


//============================================================================//
// Methods
//============================================================================//

private:

  // Convert commands into memory requests.
  void executeCommand() {
    loki_assert(!this->commandQueue.empty());

    // For the moment, we only work on one command at a time. In the future we
    // might prefer to buffer multiple sets of data.
    if (this->stagingArea.isEmpty()) {
      dma_command_t command = this->commandQueue.dequeue();

      // TODO Make this a parameter.
      MemoryOpcode memoryOp = LOAD_W;

      // Generate requests for all required values.
      for (uint col=0; col<command.rowLength; col++) {
        for (uint row=0; row<command.colLength; row++) {
          MemoryAddr addr = command.baseAddress + col*command.rowStride + row*command.colStride;
          position_t position; position.row = row; position.column = col;
          this->memoryAccess(position, addr, memoryOp);
        }
      }

    }
    else
      next_trigger(this->stagingArea.emptiedEvent());
  }

  // Receive data from memory and put it in the staging area.
  void receiveMemoryData() {
    loki_assert(this->memoryInterface.canGiveResponse());

    response_t response = this->memoryInterface.getResponse();
    this->stagingArea.write(response.position.row, response.position.column,
                            static_cast<T>(response.data));

    // To emulate parallel operations, receive the next response immediately,
    // if there is one.
    if (this->memoryInterface.canGiveResponse())
      next_trigger(sc_core::SC_ZERO_TIME);

    // If this was the final value requested, fill in any unused slots with
    // zeros. This is only valid if we only work on one command_t at a time.
    if (this->memoryInterface.isIdle())
      this->stagingArea.fillWith(0);
  }

  // Copy data from staging area to ports.
  void sendPEData() {
    loki_assert(iReadyForData.read());

    // TODO: Check that the PEs should receive this data on this tick.

    if (!this->stagingArea.isFull())
      next_trigger(this->stagingArea.filledEvent());
    else {
      for (uint row=0; row<oDataToPEs.size(); row++)
        for (uint col=0; col<oDataToPEs[row].size(); col++)
          oDataToPEs[row][col].write(this->stagingArea.read(row, col));

      oDataValid.write(true);
      // TODO: Flow control goes false when we reach a "tick" that this data
      // should not be used for.
    }
  }

};


template <typename T>
class DMAOutput: public DMABase<T> {

//============================================================================//
// Ports
//============================================================================//

public:

  // Array of values received from PEs. Addressed using array[x][y].
  LokiVector2D<sc_in<T>> iDataFromPEs;

  // Valid signal for incoming data from PEs.
  sc_in<bool>            iDataValid;

  // Flow control telling whether we're ready to receive new data.
  sc_out<bool>           oReadyForData;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMAOutput);

  DMAOutput(sc_module_name name, ComponentID id, size2d_t ports,
            size_t queueLength=4) :
      DMABase<T>(name, id, ports, queueLength),
      iDataFromPEs(ports.width, ports.height, "iDataFromPEs"),
      iDataValid("iDataValid"),
      oReadyForData("oReadyForData") {

    // Templated class means `this` must be used whenever referring to anything
    // from a parent class.

    SC_METHOD(executeCommand);
    this->sensitive << this->commandQueue.queueChangedEvent();
    this->dont_initialize();

    SC_METHOD(receivePEData);
    this->sensitive << iDataValid.pos();
    this->dont_initialize();

    SC_METHOD(receiveMemoryData);
    this->sensitive << this->memoryInterface.responseArrivedEvent();
    this->dont_initialize();

  }


//============================================================================//
// Methods
//============================================================================//

private:

  // Convert commands into memory requests.
  void executeCommand() {
    loki_assert(!this->commandQueue.empty());

    if (this->stagingArea.isFull()) {
      dma_command_t command = this->commandQueue.dequeue();

      // TODO Make this a parameter.
      MemoryOpcode memoryOp = LOAD_AND_ADD;

      // Send all desired data to memory.
      for (uint row=0; row<command.colLength; row++) {
        for (uint col=0; col<command.rowLength; col++) {
          MemoryAddr addr = command.baseAddress + col*command.rowStride + row*command.colStride;
          position_t position; position.row = row; position.column = col;
          this->memoryAccess(position, addr, memoryOp, this->stagingArea.read(row, col));

          // TODO: optionally do nothing if the output is zero
        }
      }

      // Discard any remaining data so the staging area appears empty again.
      this->stagingArea.discard();

    }
    else
      next_trigger(this->stagingArea.filledEvent());

  }

  // Receive data from PEs.
  void receivePEData() {
    loki_assert(oReadyForData.read());

    // Check to see if we've finished with the previous batch of data, and
    // wait if not.
    if (this->stagingArea.isEmpty()) {
      // Copy all data from ports into staging area.
      for (uint row=0; row<iDataFromPEs.size(); row++)
        for (uint col=0; col<iDataFromPEs[row].size(); col++)
          this->stagingArea.write(row, col, iDataFromPEs[row][col].read());

      loki_assert(this->stagingArea.isFull());
      oReadyForData.write(true);
    }
    else {
      oReadyForData.write(false);
      next_trigger(this->stagingArea.emptiedEvent());
    }
  }

  // Receive data from memory.
  void receiveMemoryData() {
    loki_assert(this->memoryInterface.canGiveResponse());

    // This unit is only responsible for sending data. If any data is received,
    // just discard it. Data might come from memory if we perform a
    // load-and-add: memory will return the previous value at that address.
    this->memoryInterface.getResponse();

    // To emulate parallel operations, receive the next response immediately,
    // if there is one.
    if (this->memoryInterface.canGiveResponse())
      next_trigger(sc_core::SC_ZERO_TIME);
  }

};


#endif /* SRC_TILE_ACCELERATOR_DMA_H_ */
/*
 * DMA.h
 *
 * Units responsible for autonomously fetching or storing blocks of data.
 *
 *  Created on: 17 Jul 2018
 *      Author: db434
 */

#ifndef SRC_TILE_ACCELERATOR_DMA_H_
#define SRC_TILE_ACCELERATOR_DMA_H_

#include "../../Exceptions/InvalidOptionException.h"
#include "../../LokiComponent.h"
#include "../../Utility/Assert.h"
#include "../../Utility/LokiVector2D.h"
#include "AcceleratorTypes.h"
#include "CommandQueue.h"
#include "ConvolutionAlgorithm.h"
#include "Interface.h"
#include "MemoryInterface.h"

class Accelerator;

// Base class for all DMA components.
class DMA: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // Command from the control unit.
  CommandInput iCommand;

  typedef sc_port<network_sink_ifc<Word>> InPort;
  typedef sc_port<network_source_ifc<Word>> OutPort;

  LokiVector<OutPort> oRequest;   // Requests sent to memory.
  LokiVector<InPort>  iResponse;  // Responses from memory.

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMA);

  DMA(sc_module_name name, ComponentID id, uint numBanks, size_t queueLength=4);


//============================================================================//
// Methods
//============================================================================//

public:

  void enqueueCommand(const dma_command_t command);

  bool canAcceptCommand() const;

  void replaceMemoryMapping(EncodedCMTEntry mapEncoded);

  // Magic connection from memory.
  void deliverDataInternal(const NetworkData& flit);

  bool isIdle() const;
  const sc_event& becameIdleEvent() const;

protected:

  // Remove and return the next command from the control unit.
  const dma_command_t dequeueCommand();

  // Generate a new memory request and send it to the appropriate memory bank.
  void createNewRequest(tick_t tick, position_t position, MemoryAddr address,
                        MemoryOpcode op, int payloadFlits, int data = 0);

  // Send a request to memory. If a response is expected, it will trigger the
  // memoryInterface.responseArrivedEvent() event.
  void memoryAccess(tick_t tick, position_t position, MemoryAddr address,
                    MemoryOpcode op, int data = 0);

  // Receive a response from memory.
  virtual void receiveMemoryData(uint interface) = 0;

  // Instantly send a request to memory. This is not a substitute for
  // memoryAccess above: this method is only to be called once all details about
  // the access have been confirmed.
  void magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address,
                         ChannelID returnChannel, Word data = 0);

  // These methods need to be overridden because their implementation depends
  // on the type of data being accessed.
  virtual void loadData(tick_t tick, position_t position, MemoryAddr address) = 0;
  virtual void storeData(tick_t tick, position_t position, MemoryAddr address, int data) = 0;
  virtual void loadAndAdd(tick_t tick, position_t position, MemoryAddr address, int data) = 0;

  Accelerator& parent() const;

private:

  void newCommandArrived();

  void detectIdleness();


//============================================================================//
// Local state
//============================================================================//

protected:

  // The ID of the command we are currently processing.
  tick_t currentTick;

  // Commands from control unit telling which data to load/store.
  CommandQueue commandQueue;

  // Component responsible for organising requests and responses from memory.
  // There is one interface for each bank on this tile, allowing maximum memory-
  // level parallelism.
  friend class MemoryInterface;
  LokiVector<MemoryInterface> memoryInterfaces;

  // Memory configuration. Tells us which memory bank to access for each memory
  // address, whether to access in cache/scratchpad mode, etc.
  ChannelMapEntry::MemoryChannel memoryMapping;

  // Fine-tunes the memory configuration for individual requests if the mapping
  // covers multiple components.
  MemoryBankSelector bankSelector;

private:

  // Event which is triggered whenever the DMA runs out of work to do.
  sc_event becameIdle;

};


// Secondary base class which contains type-dependent code.
template <typename T>
class DMABase: public DMA {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // Also include cache details?
  DMABase(sc_module_name name, ComponentID id, size2d_t ports, uint numBanks,
          size_t queueLength=4) :
      DMA(name, id, numBanks, queueLength) {

  }


//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void loadData(tick_t tick, position_t position, MemoryAddr address) {
    MemoryOpcode op;

    // The control unit doesn't know/care which type of data to access, so
    // convert a general "load" request to a load of the correct size.
    switch (sizeof(T)) {
      case 1:
        op = LOAD_B;
        break;
      case 2:
        op = LOAD_HW;
        break;
      case 4:
        op = LOAD_W;
        break;
      default:
        throw InvalidOptionException("Convolution datatype size", sizeof(T));
        break;
    }

    // Generate a memory request and send to memory.
    createNewRequest(tick, position, address, op, 0);
  }

  virtual void storeData(tick_t tick, position_t position, MemoryAddr address, int data) {
    MemoryOpcode op;

    // The control unit doesn't know/care which type of data to access, so
    // convert a general "store" request to a load of the correct size.
    switch (sizeof(T)) {
      case 1:
        op = STORE_B;
        break;
      case 2:
        op = STORE_HW;
        break;
      case 4:
        op = STORE_W;
        break;
      default:
        throw InvalidOptionException("Convolution datatype size", sizeof(T));
        break;
    }

    // Generate a memory request and send to memory.
    createNewRequest(tick, position, address, op, 1, data);
  }

  virtual void loadAndAdd(tick_t tick, position_t position, MemoryAddr address, int data) {
    int shiftedData;

    // Memory only supports load-and-adds of whole words. We can emulate the
    // operation on smaller datatypes by shifting the data to the appropriate
    // position. This only works if there is no overflow into neighbouring
    // values.
    // Note also that only integer types are supported.
    switch (sizeof(T)) {
      case 1:
        data &= 0xFF;
        shiftedData = data << ((address & 3) * 8);
        break;
      case 2:
        data &= 0xFFFF;
        shiftedData = data << ((address & 1) * 16);
        break;
      case 4:
        shiftedData = data;
        break;
      default:
        throw InvalidOptionException("Convolution datatype size", sizeof(T));
        break;
    }

    // Generate a memory request and send to memory.
    createNewRequest(tick, position, address, LOAD_AND_ADD, 1, shiftedData);
  }

};


template <typename T>
class DMAInput: public DMABase<T> {

//============================================================================//
// Ports
//============================================================================//

public:

  // Array of values sent to PEs.
  sc_port<accelerator_producer_ifc<T>> oDataToPEs;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMAInput);

  DMAInput(sc_module_name name, ComponentID id, size2d_t ports, uint numBanks,
           size_t queueLength=4) :
      DMABase<T>(name, id, ports, numBanks, queueLength),
      oDataToPEs("oDataToPEs") {

    outstandingResponses = 0;

    // Templated class means `this` must be used whenever referring to anything
    // from a parent class.

    SC_METHOD(executeCommand);
    this->sensitive << this->commandQueue.queueChangedEvent();
    this->dont_initialize();

  }

  virtual ~DMAInput() {}

private:

  // Some extra construction to happen once all ports are connected.
  virtual void end_of_elaboration() {
    SC_METHOD(manageCurrentCommand);
    this->sensitive << oDataToPEs->canWriteEvent();
    // do initialise
  }


//============================================================================//
// Methods
//============================================================================//

private:

  // Convert commands into memory requests.
  void executeCommand() {
    if (this->commandQueue.empty())
      return;

    // For the moment, we only work on one command at a time. In the future we
    // might prefer to buffer multiple sets of data.
    if (oDataToPEs->canWrite()) {
      dma_command_t command = this->commandQueue.dequeue();

      loki_assert(command.rowLength > 0);
      loki_assert(command.colLength > 0);
      LOKI_LOG(2) << this->name() << " loading " << command.rowLength
          << " columns x " << command.colLength << " rows" << endl;

      // TODO Make this a parameter.
      MemoryOpcode memoryOp = LOAD_W;

      // Generate requests for all required values.
      for (uint col=0; col<command.rowLength; col++) {
        for (uint row=0; row<command.colLength; row++) {
          MemoryAddr addr = command.baseAddress + col*command.rowStride + row*command.colStride;
          position_t position; position.row = row; position.column = col;
          this->memoryAccess(command.time, position, addr, memoryOp);
        }
      }

      // Fill in all remaining spaces with zero (for now).
      for (uint col=0; col<oDataToPEs->size().width; col++)
        for (uint row=0; row<oDataToPEs->size().height; row++)
          if (col >= command.rowLength || row >= command.colLength)
            oDataToPEs->write(row, col, 0);

      inFlight.push(command);

      // Default trigger: new command arrived
    }
    else
      next_trigger(oDataToPEs->canWriteEvent());
  }

  // Receive data from memory and forward it to the PEs.
  virtual void receiveMemoryData(uint index) {
    MemoryInterface& ifc = this->memoryInterfaces[index];

    if (!oDataToPEs->canWrite()) {
      next_trigger(oDataToPEs->canWriteEvent());
      return;
    }

    loki_assert(ifc.canGiveResponse());

    // Stall if this interface has moved on to a future tick.
    if (ifc.currentTick() != this->currentTick) {
      loki_assert(this->currentTick < ifc.currentTick());

      next_trigger(oDataToPEs->canWriteEvent());
      return;
    }

    response_t response = ifc.getResponse();
    oDataToPEs->write(response.position.row, response.position.column,
                      static_cast<T>(response.data));

    // Notify consumers when the last data has been received.
    loki_assert(outstandingResponses > 0);
    outstandingResponses--;
    if (outstandingResponses == 0) {
      oDataToPEs->finishedWriting(this->currentTick);
      LOKI_LOG(2) << this->name() << " sending data for tick " << this->currentTick << endl;
    }

    // To emulate parallel operations, receive the next response immediately,
    // if there is one.
    if (ifc.canGiveResponse())
      next_trigger(sc_core::SC_ZERO_TIME);
  }

  // There may be multiple commands in flight, so manage which one is the
  // "current" command. This is the command for which we are supplying data to
  // the PEs. We cannot accept any data for the next command until the current
  // one finishes.
  void manageCurrentCommand() {

    if (inFlight.empty()) {
      next_trigger(this->commandQueue.queueChangedEvent());
      return;
    }

    loki_assert(outstandingResponses == 0);
    loki_assert(!inFlight.empty());

    dma_command_t command = inFlight.front();
    inFlight.pop();

    this->currentTick = command.time;
    outstandingResponses = command.colLength * command.rowLength;

    // Default trigger: oDataToPEs is ready for new data.
  }

//============================================================================//
// Local state
//============================================================================//

private:

  // The commands currently in progress.
  queue<dma_command_t> inFlight;

  // Details about the data currently being sent to PEs.
  uint outstandingResponses;

};


template <typename T>
class DMAOutput: public DMABase<T> {

//============================================================================//
// Ports
//============================================================================//

public:

  // Array of values received from PEs.
  sc_port<accelerator_consumer_ifc<T>> iDataFromPEs;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(DMAOutput);

  DMAOutput(sc_module_name name, ComponentID id, size2d_t ports, uint numBanks,
            size_t queueLength=4) :
      DMABase<T>(name, id, ports, numBanks, queueLength),
      iDataFromPEs("iDataFromPEs") {

    // Templated class means `this` must be used whenever referring to anything
    // from a parent class.

    SC_METHOD(executeCommand);
    this->sensitive << this->commandQueue.queueChangedEvent();
    this->dont_initialize();

  }

  virtual ~DMAOutput() {}


//============================================================================//
// Methods
//============================================================================//

private:

  // Convert commands into memory requests.
  void executeCommand() {
    if (this->commandQueue.empty())
      return;

    if (iDataFromPEs->canRead()) {
      dma_command_t command = this->commandQueue.dequeue();
      this->currentTick = command.time;
      loki_assert(command.time == iDataFromPEs->getTick());

      // TODO Make this a parameter.
      MemoryOpcode memoryOp = LOAD_AND_ADD;

      // Send all desired data to memory.
      for (uint row=0; row<command.colLength; row++) {
        for (uint col=0; col<command.rowLength; col++) {
          MemoryAddr addr = command.baseAddress + col*command.rowStride + row*command.colStride;
          position_t position; position.row = row; position.column = col;
          T value = iDataFromPEs->read(row, col);
          this->memoryAccess(command.time, position, addr, memoryOp, value);

          // TODO: optionally do nothing if the output is zero
        }
      }

      iDataFromPEs->finishedReading();
      next_trigger(iDataFromPEs->canReadEvent());

    }
    else
      next_trigger(iDataFromPEs->canReadEvent());

  }

  // Receive data from memory.
  virtual void receiveMemoryData(uint index) {
    loki_assert(this->memoryInterfaces[index].canGiveResponse());

    // This unit is only responsible for sending data. If any data is received,
    // just discard it. Data might come from memory if we perform a
    // load-and-add: memory will return the previous value at that address.
    this->memoryInterfaces[index].getResponse();

    // To emulate parallel operations, receive the next response immediately,
    // if there is one.
    if (this->memoryInterfaces[index].canGiveResponse())
      next_trigger(sc_core::SC_ZERO_TIME);
  }

};


#endif /* SRC_TILE_ACCELERATOR_DMA_H_ */
