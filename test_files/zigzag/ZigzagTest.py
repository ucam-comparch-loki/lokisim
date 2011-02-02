#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "../../.."])
mod = imp.load_module("SimulatorTest", module, path, description)

class ZigzagTest(mod.SimulatorTest):

    def runTest(self):    
        # Read the result vector from memory 14.
        # Note that we don't write to this memory yet -- we just receive values
        # at one of the cores.
        result = self.readMemory(14, 0, 64)
        
        # See if the memory contents match the contents of the file.
        self.compare(result, filename=".expected")