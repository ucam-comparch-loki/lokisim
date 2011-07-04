#! /usr/bin/env python

# A base class for all simulator unit tests.
#
# Provides methods for interacting with the simulator: accessing arbitrary
# memory locations or registers, and executing instructions.
#
# Piggybacks on Python's existing unit testing framework, but discards some of
# the automation to give more control.

import os, string, subprocess, sys, unittest

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
        self.wait(5)
        
    # Execute a sequence of instructions without waiting for the system to
    # settle down in between.
    def executeAll(self, core, instructions):
        self._runCommand("changecore " + str(core))
        for instruction in instructions:
            self._runCommand("execute " + instruction)
        self.wait(5 + 3*len(instructions))
            
    # Wait a given number of clock cycles.
    def wait(self, numCycles):
        for i in range(1,numCycles):
            self._runCommand("")
    
    # Return the execution time so far.
    def numCycles(self):
        self._runCommand("statistic execution_time")
        return self._getOutput()[0]
        
    # Return the energy consumed to execute each useful operation.
    def energyPerOp(self):
        self._runCommand("statistic fj_per_op")
        return float(self._getOutput()[0]) / 1000
        
    # Compare a list of values with the values in the given file or in a second
    # list. Either the filename or the second list must be supplied.
    def compare(self, result, filename="", correct=[]):
        # If given a filename, populate "correct" with the file's contents.
        if filename != "":
            correct = self.fileContents(filename)
        
        # Assume that the "correct" list is now complete.
        for actual, expected in zip(result, correct):
            self.assertEqual(actual, expected)
            
    # Return the list of integers contained in the given file.
    def fileContents(self, filename):
        with open(self.find(filename)) as openfile:
            # TODO: ignore comments/blank lines
            contents = [int(line, 0) for line in openfile.readlines()]
        return contents
        
    # Do a simple search for the requested file in parent directories.
    def find(self, filename):
        directory = os.path.dirname(sys.argv[0]) + "/"
        if(os.path.exists(directory + filename)):
            return directory + filename
        elif(os.path.exists(directory + "../"+filename)):
            return directory + "../" + filename
        else:
            return directory + "../../" + filename
        
    
        
    # Tell the simulator where to load its program from.
    def _updateLoader(self, directory):
        location = self._basedir + "test_files/loader.txt"
        with open(location, "w") as loader:
            loader.write("% Loader file automatically generated by test script\n")
            #loader.write("loader " + directory + "/loader.txt\n")
            loader.write("loader params.txt\n")
            loader.write("power " + "elm")
        
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
            error = self._simulation.stderr.readlines()[4:] # Trim away SystemC intro
            errortext = "\n" + string.join(error, "")
            raise Exception(errortext)
        
    # Collect a list of lines of simulator output.
    def _getOutput(self, expectedLines=1):
        if expectedLines == -1:
            # -1 means "get all output".
            # At the moment, we can only do this if we know the program has
            # terminated, because readlines hangs otherwise.
            return self._simulation.stdout.readlines()
        else:
            output = []
            for i in range(0, expectedLines):
                try:
                    line = self._simulation.stdout.readline()
                    val = int(line)
                    output.append(val)
                except Exception as e:
                    # Found a non-numeric line, so this read didn't count.
                    i = i-1
            #output = [int(self._simulation.stdout.readline()) \
            #          for i in range(0, expectedLines)]
            return output
        
        
    # Run a single test in a subdirectory. The description can be used to
    # provide additional information (e.g. number of SIMD members).
    def test(self, directory=os.path.realpath(os.path.dirname(sys.argv[0])), description=""):        
        # basedir = where we execute the simulator from.
        self._basedir = os.getcwd().split("test_files")[0]
        if self._basedir[-1] != "/":
            self._basedir = self._basedir + "/"
    
        self._loaderFile = directory + "/loader.txt"
        
        # If the directory is a full path, strip away anything unnecessary.
        directory = directory.split("test_files/")[-1]
        self.setUp()
        
        testName = directory
        if description != "":
            testName = testName + " (" + description + ")"
        
        try:
            self.runTest()
            self._success(testName)
        except Exception as e:
            self._failure(testName, str(e))
        finally:
            self.tearDown()
            
        exit(self.returncode)
            
    # Print a message explaining why the test failed.
    def _failure(self, testName, reason):
        errormessage = reason
        if errormessage == "" or errormessage.isspace():
            errormessage = "unknown reason"
            
        red = "\033[91m"
        endColour = "\033[0m"
        firstPart = "[" + red + "FAILED" + endColour + "] " + testName
        print firstPart.ljust(49),  # colour resets position in the line?
        print "(" + errormessage + ")"
        self.returncode = 1
        
    # Print a message with information about the successful test.
    def _success(self, testName):
        firstPart = "[PASSED] " + testName
        cycles = str(self.numCycles()) + " cycles"
        energy = "%(val).1f pJ/op" % {'val':self.energyPerOp()}
        print firstPart.ljust(40),
        print cycles.ljust(14),
        print energy
        self.returncode = 0
            
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
        simulator = os.path.realpath("Debug/Loki2")
        config = os.path.realpath("test_files/loader.txt")
        
        # One "-run" loads the config file, the other loads the program.
        arguments = "test -run " + config + " -run " + self._loaderFile
        self._simulation = subprocess.Popen(simulator + " " + arguments,\
                                            shell=True,\
                                            stdin=subprocess.PIPE,\
                                            stdout=subprocess.PIPE,\
                                            stderr=subprocess.PIPE)
                
        # Execute until idle.
        try:
            self.initialExecution()
        except Exception as e:
            self.failure(str(e))
            exit(1)
         
    # Some tests may want to change the very start of execution. The default is
    # to execute until the provided program terminates, but if there is no
    # provided program, this is inappropriate.
    def initialExecution(self):
        self._runCommand("finish")
    
    # If the simulator is still running, stop it.
    def tearDown(self):
        if self._simulation.poll() == None:
            self._runCommand("quit")        
