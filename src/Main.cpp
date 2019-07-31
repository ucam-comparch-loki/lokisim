/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include <systemc.h>
#include <stdio.h>

#include "Utility/Arguments.h"
#include "Utility/BlockingInterface.h"
#include "Utility/Debugger.h"
#include "Utility/Instrumentation/IPKCache.h"
#include "Utility/Instrumentation.h"
#include "Utility/Instrumentation/Operations.h"
#include "Utility/Instrumentation/Stalls.h"
#include "Utility/Trace/Callgrind.h"
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Statistics.h"


using std::vector;
using std::string;
using Instrumentation::Stalls;


void timestep(cycle_count_t cyclesPerStep) {
  cycle_count_t cycle = Instrumentation::currentCycle();
  LOKI_LOG(1) << "\n======= Cycle " << cycle << " =======" << endl;

  sc_start((int)cyclesPerStep, SC_NS);
}

void statusUpdate(std::ostream& os) {
  os << "Current cycle number: " << Instrumentation::currentCycle()
     << " [" << Instrumentation::Operations::allOperations() << " operation(s) executed]" << endl;
}

bool checkProgress(cycle_count_t interval) {
  static count_t operationCount = 0;
  count_t newOperationCount = Instrumentation::Operations::allOperations();

  if (newOperationCount == operationCount) {
    cerr << "\nNo progress has been made for " << interval << " cycles. Aborting." << endl;

    Instrumentation::endExecution();
    BlockingInterface::reportProblems(cerr);
    RETURN_CODE = EXIT_FAILURE;

    return false;
  }

  operationCount = newOperationCount;
  return true;
}

bool checkIdle() {
  if (Stalls::cyclesIdle() >= 100) {
    statusUpdate(cerr);
    cerr << "System has been idle for " << Stalls::cyclesIdle() << " cycles. Aborting." << endl;
    Instrumentation::endExecution();
    BlockingInterface::reportProblems(cerr);
    RETURN_CODE = EXIT_FAILURE;
    return true;
  }
  return false;
}

void timeout() {
  cerr << "Simulation timed out after " << Instrumentation::currentCycle() << " cycles." << endl;
  Instrumentation::endExecution();
}

void simulate(Chip& chip, const chip_parameters_t& params) {

  cycle_count_t smallStep = 1;
  cycle_count_t bigStep = 100;
  cycle_count_t cyclesPerStep = smallStep;

  try {
    if (Debugger::usingDebugger) {
      Debugger::setChip(&chip, params);
      Debugger::waitForInput();
    }
    else {

      cycle_count_t cycle = 0;
      while (!sc_core::sc_end_of_simulation_invoked()) {

//        if (cycle >= 7000000) {
//          cyclesPerStep = 1;
//          DEBUG = 1;
//        }

        // Simulate multiple cycles in a row when possible to reduce the overheads of
        // stopping and starting simulation.
        if (DEBUG || ((cycle + bigStep) >= TIMEOUT))
          cyclesPerStep = smallStep;
        else
          cyclesPerStep = bigStep;

        if ((cycle > 0) && (cycle % 1000000 < cyclesPerStep) && !DEBUG && !Arguments::silent())
          statusUpdate(cerr);

        timestep(cyclesPerStep);
        cycle += cyclesPerStep;

        if (cycle % 100000 < cyclesPerStep) {
          bool progress = checkProgress(100000);
          if (!progress)
            break;
        }

        bool idle = checkIdle();
        if (idle)
          break;

        if (cycle >= TIMEOUT) {
          timeout();
          break;
        }

      }
    }
  }
  catch (std::exception& e) {
    // If there's no error message, it might mean that not everything is
    // connected properly.
    cerr << "Execution ended unexpectedly at cycle " << Instrumentation::currentCycle() << ":\n"
         << e.what() << endl;
    RETURN_CODE = EXIT_FAILURE;
  }

}

// Tasks which happen before the model has been generated.
void initialise(chip_parameters_t& params) {
  // Override parameters before instantiating chip model
  for (unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadParameters(Arguments::code()[i], params);
  Arguments::updateState(params);
  if (DEBUG >= 3)
    Parameters::printParameters(params);

  // Now that we know how many cores, etc, there are, initialise any
  // instrumentation structures.
  Encoding::initialise(params);
  Instrumentation::initialise(params);

  if (ENERGY_TRACE || Arguments::summarise())
    Instrumentation::start();

  // Switch off some unhelpful SystemC reports.
  sc_report_handler::set_actions("/OSCI/SystemC", SC_DO_NOTHING);
}

// Instantiate chip model - changing a parameter after this point has
// undefined behaviour.
Chip& createChipModel(const chip_parameters_t& params) {
  Chip* chip = new Chip("chip", params);
  return *chip;
}

// Tasks which happen after the chip model has been created, but before
// simulation begins.
void presimulation(Chip& chip, const chip_parameters_t& params) {
  // Put arguments for the simulated program into simulated memory.
  Arguments::storeArguments(chip);

  // Load code to execute, and link it all into one program.
  for (unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadCode(Arguments::code()[i], chip);

  CodeLoader::makeExecutable(chip);

  if (Arguments::summarise() || ENERGY_TRACE)
    Instrumentation::start();
}

// Tasks which happen after simulation finishes.
void postsimulation(Chip& chip, const chip_parameters_t& params) {
  Instrumentation::stop();

  // Print debug information
  if (Arguments::summarise())
    Instrumentation::printSummary(params);

  if (!Arguments::energyTraceFile().empty()) {
    std::ofstream output(Arguments::energyTraceFile().c_str());
    Instrumentation::dumpEventCounts(output, params);
    output.close();
    cout << "Execution statistics written to " << Arguments::energyTraceFile() << endl;
  }
  else if (Instrumentation::haveEnergyData() && RETURN_CODE == EXIT_SUCCESS) {
    // If we have collected some data but haven't been told where to put it,
    // dump it all to stdout.
    Instrumentation::dumpEventCounts(std::cout, params);
  }

  if (Arguments::ipkStats()) {
    std::ofstream output(Arguments::ipkStatsFile().c_str());
    Instrumentation::IPKCache::instructionPacketStats(output);
    output.close();
  }

  Instrumentation::end();

  // Stop traces
  Callgrind::endTrace();
}

// Entry point of my part of the program.
int sc_main(int argc, char* argv[]) {
  Arguments::parse(argc, argv);

  if (Arguments::simulate()) {
    chip_parameters_t* params = Parameters::defaultParameters();
    initialise(*params);
    Chip& chip = createChipModel(*params);
    presimulation(chip, *params);
    simulate(chip, *params);
    postsimulation(chip, *params);
    delete &chip;
    delete params;
    return RETURN_CODE;
  }
  else
    return 0;
}
