/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include <systemc.h>
#include <stdio.h>

#include "Utility/Arguments.h"
#include "Utility/Blocking.h"
#include "Utility/Debugger.h"
#include "Utility/Instrumentation.h"
#include "Utility/Instrumentation/Operations.h"
#include "Utility/Trace/Callgrind.h"
#include "Utility/Trace/CoreTrace.h"
#include "Utility/Trace/MemoryTrace.h"
#include "Utility/Trace/SoftwareTrace.h"
#include "Utility/Trace/LBTTrace.h"
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Statistics.h"

using std::vector;
using std::string;
using Instrumentation::Stalls;

// Advance the simulation cyclesPerStep clock cycles. A higher number means
// less stop-start simulation, which is quicker, but means it takes longer to
// identify any problems.
static cycle_count_t cycleNumber = 0;
static cycle_count_t cyclesPerStep = 1;

// For simplicity, 1 cycle = 1 ns
#define TIMESTEP {\
  cycleNumber += cyclesPerStep;\
  if (DEBUG) cout << "\n======= Cycle " << cycleNumber << " =======" << endl;\
  if (CORE_TRACE) CoreTrace::setClockCycle(cycleNumber);\
  if (Arguments::memoryTrace()) MemoryTrace::setClockCycle(cycleNumber);\
  if (Arguments::softwareTrace()) SoftwareTrace::setClockCycle(cycleNumber);\
  if (Arguments::lbtTrace()) LBTTrace::setClockCycle(cycleNumber);\
  sc_start((int)cyclesPerStep, SC_NS);\
}

void simulate(Chip& chip) {

  // Simulate multiple cycles in a row when possible to reduce the overheads of
  // stopping and starting simulation.
  if (DEBUG || CORE_TRACE || Arguments::memoryTrace() || Arguments::softwareTrace() || Arguments::lbtTrace())
    cyclesPerStep = 1;
  else
    cyclesPerStep = (100 < TIMEOUT/50) ? 100 : TIMEOUT/50;

  try {
    if (Debugger::usingDebugger) {
      Debugger::setChip(&chip);
      Debugger::waitForInput();
    }
    else {
      cycle_count_t cycleCounter = 0;
      count_t operationCount = 0;

      cycle_count_t i;
      for (i=0; i<TIMEOUT && !sc_core::sc_end_of_simulation_invoked(); i+=cyclesPerStep) {
        TIMESTEP;

        cycleCounter += cyclesPerStep;
        if (cycleCounter >= 1000000) {
          count_t newOperationCount = Instrumentation::Operations::numOperations();
          if (!DEBUG && !Arguments::silent())
            cerr << "Current cycle number: " << cycleNumber << " [" << newOperationCount << " operation(s) executed]" << endl;
          cycleCounter -= 1000000;

          if (newOperationCount == operationCount) {
            cerr << "\nNo progress has been made for 1000000 cycles. Aborting." << endl;

            ComponentID core0(0,0); // Assume core 0 is stalled
            fprintf(stderr, "Stuck at instruction packet at 0x%x\n",
                            chip.readRegister(core0, 1));

            Instrumentation::endExecution();
            Blocking::reportProblems(cerr);
            RETURN_CODE = EXIT_FAILURE;

            break;
          }

          operationCount = newOperationCount;
        }

//        if (cycleNumber >= 1450000) {
//          cyclesPerStep = 1;
//          DEBUG = 1;
//        }

        if (Stalls::cyclesIdle() >= 100) {
          cerr << "Current cycle number: " << cycleNumber << " [" << Instrumentation::Operations::numOperations() << " operation(s) executed]" << endl;
          cerr << "System has been idle for " << Stalls::cyclesIdle() << " cycles. Aborting." << endl;
          Instrumentation::endExecution();
          RETURN_CODE = EXIT_FAILURE;
          break;
        }
      }

      if (i >= TIMEOUT) {
        cerr << "Simulation timed out after " << TIMEOUT << " cycles." << endl;
        RETURN_CODE = EXIT_FAILURE;
      }
    }
  }
  catch (std::exception& e) {
    // If there's no error message, it might mean that not everything is
    // connected properly.
    cerr << "Execution ended unexpectedly at cycle " << sc_core::sc_time_stamp().to_default_time_units() << ":\n"
         << e.what() << endl;
    RETURN_CODE = EXIT_FAILURE;
  }

}

// Tasks which happen before the model has been generated.
void initialise() {
  // Override parameters before instantiating chip model
  for (unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadParameters(Arguments::code()[i]);
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

  // Store initial memory image
  if (Arguments::lbtTrace())
  LBTTrace::storeInitialMemoryImage(chip.getMemoryData());
}

// Tasks which happen after simulation finishes.
void postsimulation(Chip& chip) {
  // Store final memory image
  if (Arguments::lbtTrace())
  LBTTrace::storeFinalMemoryImage(chip.getMemoryData());

  // Print debug information
  if (Arguments::batchMode()) {
    Parameters::printParametersDbase();
    Statistics::printStats();
  }
  else if (Arguments::summarise() || DEBUG)
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

  Instrumentation::end();

  // Stop traces
  if (CORE_TRACE)
    CoreTrace::stop();
  if (Arguments::memoryTrace())
    MemoryTrace::stop();
  if (Arguments::softwareTrace())
    SoftwareTrace::stop();
  if (Arguments::lbtTrace())
    LBTTrace::stop();
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
