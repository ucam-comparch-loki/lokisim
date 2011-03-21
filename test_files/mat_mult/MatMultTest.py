#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "test_files"])
mod = imp.load_module("SimulatorTest", module, path, description)

class MatMultTest(mod.SimulatorTest):

    # Slice the vector into a list of rows.
    def toMatrix(self, vector, rows, columns):
        return [vector[a*columns:(a+1)*columns] for a in range(rows)]

    def runTest(self):
        # Read in the two matrices from their files. They are in list form at
        # the moment, and need to be converted into matrices.
        vector1 = self.fileContents("mat1.data")
        vector2 = self.fileContents("mat2.data")
        
        params = self.fileContents("params.data")
        
        matrix1 = self.toMatrix(vector1, params[1], params[2])
        matrix2 = self.toMatrix(vector2, params[2], params[3])
        
        # Transpose matrix2
        matrix2 = zip(*matrix2)
        
        # Compute result
        expect = []
        for row in matrix1:
            for column in matrix2:
                expect.append(sum(i*j for i,j in zip(row, column)))
                           
        # Collect result from simulator
        result = self.readMemory(11, 0, params[1]*params[3])
        
        self.compare(result, correct=expect)    
