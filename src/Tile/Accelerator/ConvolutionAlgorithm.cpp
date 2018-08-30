/*
 * ConvolutionAlgorithm.cpp
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#include "ConvolutionAlgorithm.h"
#include "../../Utility/Assert.h"

int min(int a, int b) {
  return (a > b) ? b : a;
}

ConvolutionAlgorithm::ConvolutionAlgorithm(sc_module_name name,
                                           const Configuration& config) :
    LokiComponent(name),
    config(config) {

  prepareForNewInput();

}

bool ConvolutionAlgorithm::executing() const {
  return inProgress;
}

void ConvolutionAlgorithm::start(const conv_parameters_t parameters) {
  assert(!executing());

  this->parameters = parameters;

  vector<loop_t> unordered = getUnorderedLoops(parameters);
  vector<loop_t> ordered = reorderLoops(unordered, config.loopOrder());

  loopNest = ordered;

  inProgress = true;
  startedComputationEvent.notify(sc_core::SC_ZERO_TIME);
}

void ConvolutionAlgorithm::step() {
  assert(loopNest.size() >= 2);

  // Determine the addresses of all data types in the current iteration.
  // TODO This is only valid for inference.
  MemoryAddr in1Addr = parameters.input.address;
  MemoryAddr in2Addr = parameters.filters.address;
  MemoryAddr outAddr = parameters.output.address;

  for (uint i=0; i<loopNest.size(); i++) {
    in1Addr += loopNest[i].current * loopNest[i].in1Skip;
    in2Addr += loopNest[i].current * loopNest[i].in2Skip;
    outAddr += loopNest[i].current * loopNest[i].outSkip;
  }

  // Determine how much computation is available at this position. Look at the
  // number of iterations remaining in the two innermost loops.
  loop_t& rowLoop = loopNest[loopNest.size() - 2];
  loop_t& colLoop = loopNest[loopNest.size() - 1];

  size2d_t remaining;
  remaining.width = rowLoop.iterations - rowLoop.current;
  remaining.height = colLoop.iterations - colLoop.current;

  size2d_t computeRequired;
  computeRequired.width = min(remaining.width, config.peArraySize().width);
  computeRequired.height = min(remaining.height, config.peArraySize().height);

  // Send commands
  dma_command_t in1Command;
  in1Command.baseAddress = in1Addr;
  in1Command.colLength = min(remaining.height, config.dma1Ports().height);
  in1Command.colStride = colLoop.in1Skip;
  in1Command.rowLength = min(remaining.width, config.dma1Ports().width);
  in1Command.rowStride = rowLoop.in1Skip;
  in1Command.time = stepCount;
  sendIn1Command(in1Command);

  dma_command_t in2Command;
  in2Command.baseAddress = in2Addr;
  in2Command.colLength = min(remaining.height, config.dma2Ports().height);
  in2Command.colStride = colLoop.in2Skip;
  in2Command.rowLength = min(remaining.width, config.dma2Ports().width);
  in2Command.rowStride = rowLoop.in2Skip;
  in2Command.time = stepCount;
  sendIn2Command(in2Command);

  dma_command_t outCommand;
  outCommand.baseAddress = outAddr;
  outCommand.colLength = min(remaining.height, config.dma3Ports().height);
  outCommand.colStride = colLoop.outSkip;
  outCommand.rowLength = min(remaining.width, config.dma3Ports().width);
  outCommand.rowStride = rowLoop.outSkip;
  outCommand.time = stepCount;
  sendOutCommand(outCommand);

  // Increment counters.
  stepCount++;

  // Special case for loops where we complete multiple iterations at once.
  colLoop.current += computeRequired.height;
  if (colLoop.current >= colLoop.iterations) {
    colLoop.current = 0;
    rowLoop.current += computeRequired.width;
  }

  // Default case: increase by one iteration at a time.
  uint loop = loopNest.size() - 2; // Start at rowLoop.
  while (loopNest[loop].current >= loopNest[loop].iterations) {
    loopNest[loop].current = 0;
    loop--;

    if (loop >= 0)
      loopNest[loop].current++;
    else {
      notifyExecutionFinished();
      break;
    }
  }

}

const sc_event& ConvolutionAlgorithm::startedComputation() const {
  return startedComputationEvent;
}

const sc_event& ConvolutionAlgorithm::finishedComputation() const {
  return finishedComputationEvent;
}

void ConvolutionAlgorithm::sendIn1Command(const dma_command_t command) {
//  loki_assert(!oInputCommand.valid());
  oInputCommand.write(command);
}

void ConvolutionAlgorithm::sendIn2Command(const dma_command_t command) {
//  loki_assert(!oWeightsCommand.valid());
  oWeightsCommand.write(command);
}

void ConvolutionAlgorithm::sendOutCommand(const dma_command_t command) {
//  loki_assert(!oOutputCommand.valid());
  oOutputCommand.write(command);
}

void ConvolutionAlgorithm::notifyExecutionFinished() {
  finishedComputationEvent.notify(sc_core::SC_ZERO_TIME);
  prepareForNewInput();
}

void ConvolutionAlgorithm::prepareForNewInput() {
  inProgress = false;
  stepCount = 0;
}

vector<loop_t> ConvolutionAlgorithm::getUnorderedLoops(const conv_parameters_t p) const {
  vector<loop_t> loops;

  // Group size for input and output (short name because it's used often).
  uint igs = p.shape.inChannels / p.shape.groups;
  uint ogs = p.shape.outChannels / p.shape.groups;

  // Batch.
  loop_t batch = {p.shape.batchSize, 0, p.input.batchSkip, 0, p.output.batchSkip};
  loops.push_back(batch);

  // Group.
  loop_t group = {p.shape.groups, 0, (int)igs*p.input.channelSkip, p.filters.groupSkip, (int)ogs*p.output.channelSkip};
  loops.push_back(group);

  // Input channels.
  loop_t in_c = {igs, 0, p.input.channelSkip, p.filters.inChannelSkip, 0};
  loops.push_back(in_c);

  // Output channels.
  loop_t out_c = {ogs, 0, 0, p.filters.outChannelSkip, p.output.channelSkip};
  loops.push_back(out_c);

  // Image X.
  loop_t im_x = {p.shape.imageWidth, 0, p.input.columnSkip, 0, p.output.columnSkip};
  loops.push_back(im_x);

  // Image Y.
  loop_t im_y = {p.shape.imageHeight, 0, p.input.rowSkip, 0, p.output.rowSkip};
  loops.push_back(im_y);

  // Filter X. Output stationary. (Could have input stationary instead.)
  loop_t f_x = {p.shape.filterWidth, 0, p.input.columnSkip, p.filters.columnSkip, 0};
  loops.push_back(f_x);

  // Filter Y. Output stationary. (Could have input stationary instead.)
  loop_t f_y = {p.shape.filterHeight, 0, p.input.rowSkip, p.filters.rowSkip, 0};
  loops.push_back(f_y);

  return loops;
}

vector<loop_t> ConvolutionAlgorithm::reorderLoops(const vector<loop_t>& unordered,
                                                  const LoopOrder& order) const {
  // Currently LoopOrders are fixed to 8 elements.
  assert(unordered.size() == 8);

  vector<loop_t> ordered;
  for (uint loop=0; loop<8; loop++) {
    ordered.push_back(unordered[order[loop]]);
  }

  return ordered;
}
