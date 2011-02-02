#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "../../.."])
mod = imp.load_module("SimulatorTest", module, path, description)

class FIRTest(mod.SimulatorTest):

    def runTest(self):
        # Read 415 values from memory 14.
        # 415 = input length + number of taps - 1
        result = self.readMemory(14, 0, 415)
        
        # See if the memory contents match the contents of the file.
        self.compare(result, filename=".expected")