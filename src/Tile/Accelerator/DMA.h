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
      oDataToPEs(ports.width, ports.height, "dataToPEs") {

    // Templated class means `this` must be used whenever referring to anything
    // from a parent class.

    SC_METHOD(executeCommand);
    this->sensitive << this->commandQueue.queueChangedEvent();
    this->dont_initialize();

    SC_METHOD(receiveMemoryData);
    this->sensitive << this->memoryInterface.responseArrivedEvent();
    this->dont_initialize();

    SC_METHOD(sendPEData);
    this->sensitive << iReadyForData;
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
      iDataFromPEs(ports.width, ports.height, "dataFromPEs") {

    // Templated class means `this` must be used whenever referring to anything
    // from a parent class.

    SC_METHOD(executeCommand);
    this->sensitive << this->commandQueue.queueChangedEvent();
    this->dont_initialize();

    SC_METHOD(receivePEData);
    this->sensitive << iDataValid;
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
