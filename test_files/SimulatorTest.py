#! /usr/bin/env python

# A base class for all simulator unit tests.
#
# Provides methods for interacting with the simulator: accessing arbitrary
# memory locations or registers, and executing instructions.
#
# Piggybacks on Python's existing unit testing framework, but discards some of
# the automation to give more control.

import os, subprocess, unittest

class SimulatorTest(unittest.TestCase):
            
    # Read "length" values from memory, starting at address "start".
    def readMemory(self, memory, start, length=1):
        self._runCommand("changememory " + str(memory))
        if length==1:
            self._runCommand("printmem " + str(start))
        else:
            self._runCommand("printmem " + str(start) + " +" + str(length))
        return self._getOutput(length)
        
    # Read the value of a certain core's register.
    def readReg(self, core, register):
        self._runCommand("changecore " + str(core))
        self._runCommand("printregs " + str(register))
        return self._getOutput()[0]
    
    # Read a certain core's predicate register.
    def predicate(self, core):
        self._runCommand("changecore " + str(core))
        self._runCommand("printpred")
        return self._getOutput()[0]
        
    # Execute a single instruction on a certain core.
    def execute(self, core, instruction):
        self._runCommand("changecore " + str(core))
        self._runCommand("execute " + instruction)
        self._runCommand("finish")
        
    # Execute a sequence of instructions without waiting for the system to
    # settle down in between.
    def executeAll(self, core, instructions):
        self._runCommand("changecore " + str(core))
        for instruction in instructions:
            self._runCommand("execute " + instruction)
        self._runCommand("finish")
        
    # End simulation and retrieve statistics about the execution.
    def stats(self):
        if self._simulation.poll() == None:
            self._runCommand("quit")
            self.statistics = self._getOutput(-1)
            
    # Return the execution time so far.
    def numCycles(self):
        self.stats()
        cycleLine = self.statistics[-1]
        # "Total execution time: xxx cycles" => "xxx"
        cycles = cycleLine.split(" ")[3]
        return int(cycles)
        
    # Compare a list of values with the values in the given file or in a second
    # list. Either the filename or the second list must be supplied.
    def compare(self, result, filename="", correct=[]):
        # If given a filename, populate "correct" with the file's contents.
        if filename != "":
            correct = self.fileContents(filename)
        
        # Assume that the "correct" list is now complete.
        for actual, expected in zip(result, correct):
            self.assertEqual(int(actual), expected)
            
    # Return the list of integers contained in the given file.
    def fileContents(self, filename):
        with open(self.find(filename)) as openfile:
            # TODO: ignore comments/blank lines
            contents = [int(line, 0) for line in openfile.readlines()]
        return contents
        
    # Do a simple search for the requested file in parent directories.
    def find(self, filename):
        if(os.path.exists(filename)):
            return filename
        elif(os.path.exists("../"+filename)):
            return "../" + filename
        else:
            return "../../" + filename
        
    
        
    # Tell the simulator where to load its program from.
    def _updateLoader(self, directory):
        location = self._basedir + "test_files/loader.txt"
        with open(location, "w") as loader:
            loader.write("% Loader file automatically generated by test script\n")
            loader.write("loader " + directory + "/loader.txt")
        
    # Change the values in the parameters file so that the given number of SIMD
    # members are used.
    # Assumes that the number of SIMD members is always the first value in the
    # file.
    def _setSIMDMembers(self, numMembers):
        paramsLoc = self.find("params.data")
        with open(paramsLoc, "r+") as paramsFile:
            # Replaces characters (including new lines), so need ljust to pad
            # to correct length.
            paramsFile.write(str(numMembers).ljust(2) + "\n")

    # Pass a command to the simulator.
    def _runCommand(self, command):
        if self._simulation.poll() == None:
            self._simulation.stdin.write(command + "\n")
        else:
            raise Exception("simulation ended unexpectedly")
        
    # Collect a list of lines of simulator output.
    def _getOutput(self, expectedLines=1):
        if expectedLines == -1:
            # -1 means "get all output".
            # At the moment, we can only do this if we know the program has
            # terminated, because readlines hangs otherwise.
            return self._simulation.stdout.readlines()
        else:
            output = [int(self._simulation.stdout.readline()) \
                      for i in range(0, expectedLines)]
            return output
        
        
    # Run a single test in a subdirectory. The description can be used to
    # provide additional information (e.g. number of SIMD members).
    def test(self, directory=os.getcwd(), description=""):
        # currdir = where we execute the test from
        self._currdir = os.getcwd()
        
        # basedir = where we execute the simulator from.
        self._basedir = os.getcwd().split("test_files")[0]
    
        # If the directory is a full path, strip away anything unnecessary.
        directory = directory.split("test_files/")[-1]
        self._updateLoader(directory)
        self.setUp()
        
        message = "Testing " + directory
        if(description != ""):
            message += (" (" + description + ")")
        message += "..."
        print message.ljust(50),    # Pad the message so everything lines up
        
        try:
            self.runTest()
        except Exception as e:
            red = "\033[91m"
            endColour = "\033[0m"
            print(red + "failed" + endColour + " (" + str(e) + ")")
        else:
            print("passed (" + str(self.numCycles()) + " cycles)")
        finally:
            self.tearDown()
            
    # The default behaviour is to execute the test in the current directory.
    # This should be overridden in subclasses to allow the same test to be run
    # multiple times with different parameters.
    def runAllTests(self):
        self.test()
        
    # Run a SIMD test for various numbers of SIMD members. Tries every value
    # between startCores and endCores, inclusive.
    def simdTest(self, startCores, endCores):
        for simdMembers in range(startCores, endCores+1):
            self._setSIMDMembers(simdMembers)
            self.test(description = str(simdMembers) + " cores")
    
    # Start up the simulator in test mode.
    def setUp(self):
        os.chdir(self._basedir)
        
        location = "Debug/Loki2"
        arguments = "test"
        self._simulation = subprocess.Popen(location + " " + arguments,\
                                            shell=True,\
                                            stdin=subprocess.PIPE,\
                                            stdout=subprocess.PIPE,\
                                            stderr=subprocess.PIPE)
                                            
        os.chdir(self._currdir)
        
        # Execute until idle.           
        self._runCommand("finish")
    
    # If the simulator is still running, stop it.
    def tearDown(self):
        if self._simulation.poll() == None:
            self._runCommand("quit")        
