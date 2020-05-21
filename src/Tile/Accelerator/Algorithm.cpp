/*
 * Algorithm.cpp
 *
 *  Created on: 23 Jul 2018
 *      Author: db434
 */

#include "Algorithm.h"

#include "../../Utility/Assert.h"

int min(int a, int b) {
  return (a > b) ? b : a;
}

Algorithm::Algorithm(sc_module_name name,
                                           const accelerator_parameters_t& config) :
    LokiComponent(name),
    config(config) {

  prepareForNewInput();

}

bool Algorithm::executing() const {
  return inProgress;
}

void Algorithm::start(const lat_parameters_t parameters) {
  loki_assert(!executing());

  this->parameters = parameters;
  activePEs = config.numPEs;

  loopNest = getLoops(parameters);
  checkLoops(loopNest);

  inProgress = true;
  startedComputationEvent.notify(sc_core::SC_ZERO_TIME);

  // Check for corner case where one of the loops has no iterations to do.
  for (uint i=0; i<loopNest.size(); i++) {
    if (loopNest[i].iterations == 0) {
//      LOKI_WARN << this->name() << " asked to perform computation with 0 iterations" << endl;
      notifyExecutionFinished();
      break;
    }
  }
}

void Algorithm::step() {
  loki_assert_with_message(loopNest.size() >= 2, "Size = %d", loopNest.size());

  // Determine the addresses of all data types in the current iteration.
  MemoryAddr in1Addr = parameters.in1.address;
  MemoryAddr in2Addr = parameters.in2.address;
  MemoryAddr outAddr = parameters.out.address;

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
  computeRequired.width = min(remaining.width, activePEs.width);
  computeRequired.height = min(remaining.height, activePEs.height);

  // Send commands
  dma_command_t in1Command;
  in1Command.baseAddress = in1Addr;
  in1Command.colLength = min(computeRequired.height, config.dma1Ports().height);
  in1Command.colStride = colLoop.in1Skip;
  in1Command.rowLength = min(computeRequired.width, config.dma1Ports().width);
  in1Command.rowStride = rowLoop.in1Skip;
  in1Command.time = stepCount;
  sendIn1Command(in1Command);

  dma_command_t in2Command;
  in2Command.baseAddress = in2Addr;
  in2Command.colLength = min(computeRequired.height, config.dma2Ports().height);
  in2Command.colStride = colLoop.in2Skip;
  in2Command.rowLength = min(computeRequired.width, config.dma2Ports().width);
  in2Command.rowStride = rowLoop.in2Skip;
  in2Command.time = stepCount;
  sendIn2Command(in2Command);

  dma_command_t outCommand;
  outCommand.baseAddress = outAddr;
  outCommand.colLength = min(computeRequired.height, config.dma3Ports().height);
  outCommand.colStride = colLoop.outSkip;
  outCommand.rowLength = min(computeRequired.width, config.dma3Ports().width);
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
  int loop = loopNest.size() - 2; // Start at rowLoop.
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

const sc_event& Algorithm::startedComputation() const {
  return startedComputationEvent;
}

const sc_event& Algorithm::finishedComputation() const {
  return finishedComputationEvent;
}

void Algorithm::sendIn1Command(const dma_command_t command) {
//  loki_assert(!oInputCommand.valid());
  oInputCommand.write(command);
}

void Algorithm::sendIn2Command(const dma_command_t command) {
//  loki_assert(!oWeightsCommand.valid());
  oWeightsCommand.write(command);
}

void Algorithm::sendOutCommand(const dma_command_t command) {
//  loki_assert(!oOutputCommand.valid());
  oOutputCommand.write(command);
}

void Algorithm::notifyExecutionFinished() {
  finishedComputationEvent.notify(sc_core::SC_ZERO_TIME);
  prepareForNewInput();
}

void Algorithm::prepareForNewInput() {
  inProgress = false;
  stepCount = 0;
}

// Determine how large the output will be (in pixels), given the computation
// parameters. This applies to any windowed computation, e.g. convolution,
// pooling.
// The equation is taken from PyTorch (minus the `padding` parameter):
// https://pytorch.org/docs/stable/nn.html#conv2d
uint outputSize(uint inputSize, uint windowSize, uint stride, uint dilation) {
  int size = ((inputSize - dilation * (windowSize - 1) - 1) / stride) + 1;

  return (size < 0) ? 0 : size;
}

vector<loop_t> Algorithm::getLoops(const lat_parameters_t& p) const {
  loki_assert(p.loop_count == p.loops.size());
  loki_assert(p.loop_count == p.iteration_counts.size());

  vector<loop_t> loops;

  for (uint i=0; i<p.loop_count; i++) {
    loop_t loop;
    loop.iterations = p.iteration_counts[i];
    loop.current = 0;
    loop.in1Skip = p.loops[i].in1_stride;
    loop.in2Skip = p.loops[i].in2_stride;
    loop.outSkip = p.loops[i].out_stride;

    loops.push_back(loop);
  }

  return loops;
}


// Check to make sure the requested loop order is compatible with the
// accelerator's broadcast/accumulate configuration.
void Algorithm::checkLoops(const vector<loop_t>& loops) {
  // The final two loops are parallelised on the accelerator.
  const loop_t& rowLoop = loops[loops.size()-2]; // Iterates along a row of PEs
  const loop_t& colLoop = loops[loops.size()-1]; // Iterates along a column of PEs

  // If an input is broadcast, the loop parallelised along that dimension should
  // treat that input as constant (i.e. stride = 0).
  if (config.broadcastRows && rowLoop.in1Skip != 0) {
    LOKI_WARN << this->name() << " received incompatible computation request." << endl;
    LOKI_WARN << "  Unable to broadcast along rows of PEs: using first column only." << endl;
    LOKI_WARN << "  --acc-broadcast-rows=1 but penultimate loop doesn't treat input 1 as constant." << endl;

    // Set columns to 1.
    activePEs.width = 1;
  }

  if (config.broadcastCols && colLoop.in2Skip != 0) {
    LOKI_WARN << this->name() << " received incompatible computation request." << endl;
    LOKI_WARN << "  Unable to broadcast along columns of PEs: using first row only." << endl;
    LOKI_WARN << "  --acc-broadcast-cols=1 but final loop doesn't treat input 2 as constant." << endl;

    // Set rows to 1.
    activePEs.height = 1;
  }

  // If the output is accumulated, the loop parallelised in that dimension
  // should treat the output as constant (i.e. stride = 0).
  if (config.accumulateRows && rowLoop.outSkip != 0) {
    LOKI_WARN << this->name() << " received incompatible computation request." << endl;
    LOKI_WARN << "  Unable to accumulate along rows of PEs: using first column only." << endl;
    LOKI_WARN << "  --acc-accumulate-rows=1 but penultimate loop doesn't treat output as constant." << endl;

    // Set columns to 1.
    activePEs.width = 1;
  }

  if (config.accumulateCols && colLoop.outSkip != 0) {
    LOKI_WARN << this->name() << " received incompatible computation request." << endl;
    LOKI_WARN << "  Unable to accumulate along columns of PEs: using first row only." << endl;
    LOKI_WARN << "  --acc-accumulate-cols=1 but final loop doesn't treat output as constant." << endl;

    // Set rows to 1.
    activePEs.height = 1;
  }
}
