/*
 * EnergyTrace.h
 *
 * Record all events which consume a significant amount of energy.
 *
 * In particular, we record *events* (e.g. data sent from core x to core y) and
 * not their consequences (e.g. bits switched). This allows us to recompute
 * energy for some configurations without having to simulate the program again.
 *
 * Data is written in a binary format, as these traces can get very long.
 *
 * A further processing stage is to be carried out after simulation has finished
 * to convert these high level events into low level information. This low level
 * information can then be combined with an energy model in a final stage to
 * compute the total energy consumed.
 *
 *  Created on: 2 Apr 2012
 *      Author: db434
 */

#ifndef ENERGYTRACEWRITER_H_
#define ENERGYTRACEWRITER_H_

#include <iostream>
#include "../../Datatype/ChannelID.h"
#include "../../Datatype/ComponentID.h"
#include "../../Datatype/DecodedInst.h"

class EnergyTraceWriter {

//==============================//
// Methods
//==============================//

public:

  static void start(const std::string& filename);
  static void stop();

  static void setOutputStream(std::ostream *stream);

  static void fifoRead(uint8_t fifoSize);
  static void fifoWrite(uint8_t fifoSize);
  static void registerRead(ComponentID core, RegisterIndex reg, bool indirect);
  static void registerWrite(ComponentID core, RegisterIndex reg, bool indirect);
  static void memoryRead(uint32_t memSizeInBytes);
  static void memoryWrite(uint32_t memSizeInBytes);
  static void tagCheck(uint32_t numTags, uint8_t tagWidthInBits);
  static void decode(const DecodedInst& inst);
  static void execute(const DecodedInst& inst);
  static void dataBypass(ComponentID core, int32_t data);
  static void networkTraffic(ComponentID source, ChannelID destination, uint32_t data);
  static void arbitration(uint8_t numInputs);

private:

  // Write bytes from the event union to the output stream.
  static void write(const size_t bytes);

//==============================//
// Local state
//==============================//

private:

  // The stream to write the trace to.
  static std::ostream *output;

};

#endif /* ENERGYTRACEWRITER_H_ */
