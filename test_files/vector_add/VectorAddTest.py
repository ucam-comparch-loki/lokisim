#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "test_files"])
mod = imp.load_module("SimulatorTest", module, path, description)

class VectorAddTest(mod.SimulatorTest):

    def runTest(self):
        # Read in the two vectors from their files.
        vector1 = self.fileContents("vector1.data")
        vector2 = self.fileContents("vector2.data")
        
        # Compute the result we expect.
        expected = [x + y for x,y in zip(vector1, vector2)]
    
        # Read the result vector from memory.
        result = self.readMemory(8, 0x20000, len(expected))
        
        # See if the memory contents match the values we computed.
        self.compare(result, correct=expected)
