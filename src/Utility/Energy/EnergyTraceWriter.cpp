/*
 * EnergyTrace.cpp
 *
 *  Created on: 2 Apr 2012
 *      Author: db434
 */

#include "EnergyTraceWriter.h"
#include "EventTypes.h"
#include <fstream>
//#include <boost/iostreams/filter/bzip2.hpp>
//#include <boost/iostreams/filtering_stream.hpp>

std::ostream* EnergyTraceWriter::output = NULL;
EventData writeEvent;

void EnergyTraceWriter::start(const std::string& filename) {
  if(output != NULL)
    cerr << "Warning: already called EnergyTraceWriter::start()" << endl;

  setOutputStream(new std::ofstream(filename.c_str(), std::ios_base::binary));

  // TODO: output parameters?
}

void EnergyTraceWriter::stop() {
  if(output != NULL) {
    output->flush();
    static_cast<std::ofstream*>(output)->close();
    delete output;
  }

  output = NULL;
}

void EnergyTraceWriter::setOutputStream(std::ostream *stream) {
  // Add a boost filter to the output stream so that data is compressed on its
  // way out.
  // Using this requires the libboost-iostreams1.42-dev package and
  // boost_iostreams on the library list.
  // Currently unused because the zip takes about twice as long as the simulation.
//  boost::iostreams::filtering_stream<boost::iostreams::output> *my_filter =
//    new boost::iostreams::filtering_stream<boost::iostreams::output>();
//  my_filter->push(boost::iostreams::bzip2_compressor()) ;
//  my_filter->push(*stream);
//  output = my_filter;

  output = stream;
}

void EnergyTraceWriter::fifoRead(uint8_t fifoSize) {
  if(output == NULL) return;

  writeEvent.type = FIFO_READ;
  writeEvent.fifoEvent.fifoSize = fifoSize;
  write(sizeof(FifoEvent));
}

void EnergyTraceWriter::fifoWrite(uint8_t fifoSize) {
  if(output == NULL) return;

  writeEvent.type = FIFO_WRITE;
  writeEvent.fifoEvent.fifoSize = fifoSize;
  write(sizeof(FifoEvent));
}

void EnergyTraceWriter::registerRead(ComponentID core, RegisterIndex reg, bool indirect) {
  if(output == NULL) return;

  writeEvent.type = REGISTER_READ;
  writeEvent.registerEvent.tile = core.getTile();
  writeEvent.registerEvent.core = core.getPosition();
  writeEvent.registerEvent.reg = reg;
  writeEvent.registerEvent.indirect = indirect;
  write(sizeof(RegisterEvent));
}

void EnergyTraceWriter::registerWrite(ComponentID core, RegisterIndex reg, bool indirect) {
  if(output == NULL) return;

  writeEvent.type = REGISTER_WRITE;
  writeEvent.registerEvent.tile = core.getTile();
  writeEvent.registerEvent.core = core.getPosition();
  writeEvent.registerEvent.reg = reg;
  writeEvent.registerEvent.indirect = indirect;
  write(sizeof(RegisterEvent));
}

void EnergyTraceWriter::memoryRead(uint32_t memSizeInBytes) {
  if(output == NULL) return;

  writeEvent.type = MEMORY_READ;
  writeEvent.memoryEvent.memSizeInBytes = memSizeInBytes;
  write(sizeof(MemoryEvent));
}

void EnergyTraceWriter::memoryWrite(uint32_t memSizeInBytes) {
  if(output == NULL) return;

  writeEvent.type = MEMORY_WRITE;
  writeEvent.memoryEvent.memSizeInBytes = memSizeInBytes;
  write(sizeof(MemoryEvent));
}

void EnergyTraceWriter::tagCheck(uint32_t numTags, uint8_t tagWidthInBits) {
  if(output == NULL) return;

  writeEvent.type = TAG_CHECK;
  writeEvent.tagCheckEvent.numTags = numTags;
  writeEvent.tagCheckEvent.tagWidth = tagWidthInBits;
  write(sizeof(TagCheckEvent));
}

void EnergyTraceWriter::decode(const DecodedInst& inst) {
  if(output == NULL) return;

  writeEvent.type = DECODE;
  writeEvent.decodeEvent.operation = inst.opcode();
  write(sizeof(DecodeEvent));
}

void EnergyTraceWriter::execute(const DecodedInst& inst) {
  if(output == NULL) return;

  writeEvent.type = EXECUTE;
  writeEvent.executeEvent.operation = inst.opcode();
  writeEvent.executeEvent.function = inst.function();
  writeEvent.executeEvent.operand1 = inst.operand1();
  writeEvent.executeEvent.operand2 = inst.operand2();
  write(sizeof(ExecuteEvent));
}

void EnergyTraceWriter::dataBypass(ComponentID core, int32_t data) {
  if(output == NULL) return;

  writeEvent.type = DATA_BYPASS;
  writeEvent.bypassEvent.tile = core.getTile();
  writeEvent.bypassEvent.core = core.getPosition();
  writeEvent.bypassEvent.data = data;
  write(sizeof(BypassEvent));
}

void EnergyTraceWriter::networkTraffic(ComponentID source, ChannelID destination, uint32_t data) {
  if(output == NULL) return;

  writeEvent.type = NETWORK_TRAFFIC;
  writeEvent.networkEvent.tileSource = source.getTile();
  writeEvent.networkEvent.componentSource = source.getPosition();
  writeEvent.networkEvent.tileDestination = destination.getTile();
  writeEvent.networkEvent.componentDestination = destination.getPosition();
  writeEvent.networkEvent.channelDestination = destination.getChannel();
  writeEvent.networkEvent.data = data;
  write(sizeof(NetworkEvent));
}

void EnergyTraceWriter::arbitration(uint8_t numInputs) {
  if(output == NULL) return;

  writeEvent.type = ARBITRATION;
  writeEvent.arbitrationEvent.inputs = numInputs;
  write(sizeof(ArbitrationEvent));
}

void EnergyTraceWriter::write(const size_t bytes) {
  output->write(writeEvent.rawData, bytes + sizeof(EnergyEvent));
}
