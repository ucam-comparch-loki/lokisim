/*
 * Main.cpp
 *
 *  Created on: 13 Jan 2010
 *      Author: db434
 */

#include <systemc.h>
#include <stdio.h>
#include "Utility/Arguments.h"
#include "Utility/Debugger.h"
#include "Utility/Instrumentation.h"
#include "Utility/Instrumentation/Operations.h"
#include "Utility/Trace/CoreTrace.h"
#include "Utility/Trace/MemoryTrace.h"
#include "Utility/StartUp/CodeLoader.h"
#include "Utility/Statistics.h"

using std::vector;
using std::string;

// Advance the simulation one clock cycle.
static unsigned long long cycleNumber = 0;
static unsigned int cyclesPerStep = 1;

// For simplicity, 1 cycle = 1 ns
#define TIMESTEP {\
  cycleNumber += cyclesPerStep;\
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber << " =======" << endl;\
  if(CORE_TRACE) CoreTrace::setClockCycle(cycleNumber);\
  if(MEMORY_TRACE) MemoryTrace::setClockCycle(cycleNumber);\
  sc_start(cyclesPerStep, SC_NS);\
}

int simulate();

int sc_main(int argc, char* argv[]) {
  Arguments::parse(argc, argv);

  if(Arguments::simulate())
    return simulate();
  else
    return 0;
}

void simulate(Chip& chip) {

  // Simulate multiple cycles in a row when possible to reduce the overheads of
  // stopping and starting simulation.
  if(DEBUG || CORE_TRACE || MEMORY_TRACE)
    cyclesPerStep = 1;
  else
    cyclesPerStep = (100 < TIMEOUT/50) ? 100 : TIMEOUT/50;

  try {
    if(Debugger::usingDebugger) {
      Debugger::setChip(&chip);
      Debugger::waitForInput();
    }
    else {
      int cyclesIdle = 0;
      int cycleCounter = 0;
      int operationCount = 0;

      unsigned long long i;
      for(i=0; i<TIMEOUT && !sc_core::sc_end_of_simulation_invoked(); i+=cyclesPerStep) {
        TIMESTEP;

        cycleCounter += cyclesPerStep;
        if (cycleCounter >= 1000000) {
          int newOperationCount = Instrumentation::Operations::numOperations();
          if(!DEBUG)
            cerr << "Current cycle number: " << cycleNumber << " [" << newOperationCount << " operation(s) executed]" << endl;
          cycleCounter -= 1000000;

          if(newOperationCount == operationCount) {
            cerr << "\nNo progress has been made for 1000000 cycles. Aborting." << endl;

            ComponentID core0(0,0); // Assume core 0 is stalled
            fprintf(stderr, "Stuck at instruction packet at 0x%x\n",
                            chip.readRegister(core0, 1));

            Instrumentation::endExecution();
            RETURN_CODE = EXIT_FAILURE;

            break;
          }

          operationCount = newOperationCount;
        }

        if(chip.isIdle()) {
          cyclesIdle++;
          if(cyclesIdle >= 100) {
            cerr << "\nSystem has been idle for " << cyclesIdle << " cycles. Aborting." << endl;
            sc_stop();
            Instrumentation::endExecution();
            RETURN_CODE = EXIT_FAILURE;
            break;
          }
        }
        else cyclesIdle = 0;
      }

      if(i >= TIMEOUT) {
        cerr << "Simulation timed out after " << TIMEOUT << " cycles." << endl;
        RETURN_CODE = EXIT_FAILURE;
      }
    }
  }
  catch(std::exception& e) {
    // If there's no error message, it might mean that not everything is
    // connected properly.
    cerr << "Execution ended unexpectedly at cycle " << sc_core::sc_time_stamp().to_double() << ":\n"
         << e.what() << endl;
    RETURN_CODE = EXIT_FAILURE;
  }

}

// Wrapper for the chip simulation function above.
int simulate() {
  // Override parameters before instantiating chip model
  for(unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadParameters(Arguments::code()[i]);

  Instrumentation::initialise();

  // Instantiate chip model - changing a parameter after this point has
  // undefined behaviour.
  Chip chip("chip", 0);

  // Put arguments for the simulated program into simulated memory.
  Arguments::storeArguments(chip);

  // Load code to execute, and link it all into one program.
  for(unsigned int i=0; i<Arguments::code().size(); i++)
    CodeLoader::loadCode(Arguments::code()[i], chip);

  CodeLoader::makeExecutable(chip);

  // Run simulation
  simulate(chip);

  // Print debug information
  if (DEBUG || BATCH_MODE) {
    Parameters::printParameters();
    Statistics::printStats();
  }

  if (!Arguments::energyTraceFile().empty()) {
    std::ofstream output(Arguments::energyTraceFile().c_str());
    Instrumentation::dumpEventCounts(output);
    output.close();
    cout << "Execution statistics written to " << Arguments::energyTraceFile() << endl;
  }

  Instrumentation::end();

  // Stop traces
  if (CORE_TRACE)
    CoreTrace::stop();
  if (MEMORY_TRACE)
    MemoryTrace::stop();

  return RETURN_CODE;
}
