#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "test_files"])
mod = imp.load_module("SimulatorTest", module, path, description)

class TransposeTest(mod.SimulatorTest):

    # Slice the vector into a list of rows.
    def toMatrix(self, vector, rows, columns):
        return [vector[a*columns:(a+1)*columns] for a in range(rows)]

    def runTest(self):        
        params = self.fileContents("params.data")
        
        # Read in the matrix from its file. It is in list form at the moment, 
        # and needs to be converted into a matrix.
        vector = self.fileContents("matrix.data")        
        matrix = self.toMatrix(vector, params[1], params[2])
        
        # Transpose the matrix
        matrix = zip(*matrix)
        
        # Flatten the matrix into a list
        matrix = [element for sublist in matrix for element in sublist]
        
        # Collect result from simulator
        result = self.readMemory(8, 0x20000, params[1]*params[2])
        
        self.compare(result, correct=matrix)    
