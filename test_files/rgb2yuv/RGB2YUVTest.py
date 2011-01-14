#! /usr/bin/env python

import ctypes, imp

module, path, description = imp.find_module("SimulatorTest", ["..", "../..", "../../.."])
mod = imp.load_module("SimulatorTest", module, path, description)

class RGB2YUVTest(mod.SimulatorTest):

    # Converts a list of 32 bit integers to a list of bytes.
    # Big-endian (?): 0x12345678 becomes [0x12, 0x34, 0x56, 0x78]
    def toBytes(self, ints):
        bytes = []
        for i in ints:
            # Convert from signed to unsigned
            uint = ctypes.c_uint32(i).value
            bytes.append((uint >> 24) & 255)
            bytes.append((uint >> 16) & 255)
            bytes.append((uint >>  8) & 255)
            bytes.append((uint >>  0) & 255)
        return bytes

    # Read the RGB files and return the pixels in a triple.
    def getPixels(self):
        red   = self.toBytes(self.fileContents("red.data"))
        green = self.toBytes(self.fileContents("green.data"))
        blue  = self.toBytes(self.fileContents("blue.data"))
        return zip(red,green,blue)

    def runTest(self):
        pixels = self.getPixels()
        
        # Compute result
        # If 4:2:0 subsampling is ever used, use pixels[::4] for U and V to only
        # use every fourth element.
        expectY = [(( 66*r + 129*g +  25*b + 128) >> 8) +  16 for (r,g,b) in pixels]
        expectU = [((-38*r -  74*g + 112*b + 128) >> 8) + 128 for (r,g,b) in pixels]
        expectV = [((112*r -  94*g -  18*b + 128) >> 8) + 128 for (r,g,b) in pixels]
        
        # Number of words to collect from the simulator for each output channel.
        length = len(pixels)/4
                           
        # Collect result from simulator
        actualY = self.toBytes(self.readMemory(14, 0, length))
        actualU = self.toBytes(self.readMemory(14, length*4, length))
        actualV = self.toBytes(self.readMemory(14, length*8, length))
        
        self.compare(actualY, correct=expectY)
        self.compare(actualU, correct=expectU)
        self.compare(actualV, correct=expectV)
