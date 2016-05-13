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
#include "Utility/Trace/Callgrind.h"
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Statistics.h"


using std::vector;
using std::string;
using Instrumentation::Stalls;


void timestep(cycle_count_t cyclesPerStep) {
  cycle_count_t cycle = Instrumentation::currentCycle();
  if (DEBUG) cout << "\n======= Cycle " << cycle << " =======" << endl;

  sc_start((int)cyclesPerStep, SC_NS);
}

void statusUpdate(std::ostream& os) {
  os << "Current cycle number: " << Instrumentation::currentCycle()
     << " [" << Instrumentation::Operations::numOperations() << " operation(s) executed]" << endl;
}

bool checkProgress(cycle_count_t interval) {
  static count_t operationCount = 0;
  count_t newOperationCount = Instrumentation::Operations::numOperations();

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

void simulate(Chip& chip) {

  cycle_count_t cyclesPerStep;

  // Simulate multiple cycles in a row when possible to reduce the overheads of
  // stopping and starting simulation.
  if (DEBUG)
    cyclesPerStep = 1;
  else
    cyclesPerStep = 100;

  try {
    if (Debugger::usingDebugger) {
      Debugger::setChip(&chip);
      Debugger::waitForInput();
    }
    else {

      cycle_count_t cycle = 0;
      while (!sc_core::sc_end_of_simulation_invoked()) {

//        if (cycle >= 7000000) {
//          cyclesPerStep = 1;
//          DEBUG = 1;
//        }

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
void initialise() {
  // Override parameters before instantiating chip model
  for (unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadParameters(Arguments::code()[i]);
  Arguments::setCommandLineParameters();
  if (DEBUG)
    Parameters::printParameters();

  // Now that we know how many cores, etc, there are, initialise any
  // instrumentation structures.
  Instrumentation::initialise();

  // Switch off some unhelpful SystemC reports.
  sc_report_handler::set_actions("/OSCI/SystemC", SC_DO_NOTHING);
}

// Instantiate chip model - changing a parameter after this point has
// undefined behaviour.
Chip& createChipModel() {
  Chip* chip = new Chip("chip", 0);
  return *chip;
}

// Tasks which happen after the chip model has been created, but before
// simulation begins.
void presimulation(Chip& chip) {
  // Put arguments for the simulated program into simulated memory.
  Arguments::storeArguments(chip);

  // Load code to execute, and link it all into one program.
  for (unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadCode(Arguments::code()[i], chip);

  CodeLoader::makeExecutable(chip);

  if (ENERGY_TRACE)
    Instrumentation::startEventLog();
}

// Tasks which happen after simulation finishes.
void postsimulation(Chip& chip) {
  // Print debug information
  if (Arguments::summarise() || DEBUG)
    Instrumentation::printSummary();

  Instrumentation::stopEventLog();

  if (!Arguments::energyTraceFile().empty()) {
    std::ofstream output(Arguments::energyTraceFile().c_str());
    Instrumentation::dumpEventCounts(output);
    output.close();
    cout << "Execution statistics written to " << Arguments::energyTraceFile() << endl;
  }
  else if (Instrumentation::haveEnergyData() && RETURN_CODE == EXIT_SUCCESS) {
    // If we have collected some data but haven't been told where to put it,
    // dump it all to stdout.
    Instrumentation::dumpEventCounts(std::cout);
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
    initialise();
    Chip& chip = createChipModel();
    presimulation(chip);
    simulate(chip);
    postsimulation(chip);
    delete &chip;
    return RETURN_CODE;
  }
  else
    return 0;
}
