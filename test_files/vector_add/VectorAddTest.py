#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "../../.."])
mod = imp.load_module("SimulatorTest", module, path, description)

class VectorAddTest(mod.SimulatorTest):

    def runTest(self):
        # Read in the two vectors from their files.
        vector1 = self.fileContents("vector1.data")
        vector2 = self.fileContents("vector2.data")
        
        # Compute the result we expect.
        expected = [x + y for x,y in zip(vector1, vector2)]
    
        # Read the result vector from memory 15.
        result = self.readMemory(15, 0, len(expected))
        
        # See if the memory contents match the contents of the file.
        self.compare(result, correct=expected)
        
#    def runAllTests(self):
#        self.test("vector_add/1_core")
#        
#        # Could change parameters here and try SIMD with several configurations
#        simdMembers = self.fileContents("params.data")[0]
#        self.test("vector_add/simd", str(simdMembers) + " cores")
#            
#if __name__ == '__main__':
#    VectorAddTest().runAllTests()
