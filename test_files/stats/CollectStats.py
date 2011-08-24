#! /usr/bin/env python

import imp

module, path, description = imp.find_module("SimulatorTest", ["..", "test_files"])
mod = imp.load_module("SimulatorTest", module, path, description)

# The aim of this class is to execute individual or very short sequences of
# instructions, and determine how long common operations take.
class CollectStats(mod.SimulatorTest):        
        
    # Don't bother executing until idle before starting this test.
    def initialExecution(self):
        # Wait 25 cycles until initialisation program finishes
        self.wait(60)
        
    # Don't want a pass message
    def _success(self, testName):
        self.returncode = 0
        
    # Don't want a failure message
    def _failure(self, testName, reason):
        self.returncode = 1
        
        errormessage = reason
        if errormessage == "" or errormessage.isspace():
            errormessage = "unknown reason"
        
        print "Error: unable to collect statistics:", errormessage
        
    def currentCycle(self):
        return self.getStat("current_cycle")
        
    # Execute a single instruction on a certain core, but don't execute until
    # it's finished - we want to time it.
    def execute(self, core, instruction):
        self._runCommand("changecore " + str(core))
        self._runCommand("execute " + instruction)

    def runTest(self):
        self.findPipelineLatency()
        self.findLocalNetworkLatency()
        self.findMemoryLatency()
        self.findRouterLatency()
        
    # Execute until the condition is satisfied. Return how many cycles it took.
    def waitUntil(self, condition):
        timeWaited = 0
        while (not condition()) and (timeWaited < 1000):
            self.wait(1)
            timeWaited += 1
        return timeWaited
    
    # Wait until the condition is satisfied, and then print out a message.
    # Offset is a value to be used if several things are going on at once, and
    # we already know how long most of them take. It allows the known latencies
    # to be removed, leaving only the latency of interest.
    def findLatency(self, name, condition, offset=0):
        timeWaited = self.waitUntil(condition)        
        print name, "latency:\t", (timeWaited-offset), "cycles"
        return timeWaited-offset
        
    def findPipelineLatency(self):        
        self.execute(0, "ori.eop r10 r0 1337")
        self.pipelineLatency = self.findLatency("Pipeline",
                                                lambda: self.readReg(0,10) == 1337)
        
    def findLocalNetworkLatency(self):
        self.execute(0, "ori r2 r0 (0,1,2)")
        self.execute(0, "setchmap 2 r2")
        self.execute(1, "or.eop r10 ch0 r0")
        self.wait(10)
        self.execute(0, "ori.eop r0 r0 1337 -> 2")
        
        # Before the value appears in the remote core's register, the instruction
        # must pass right through core 0, and the received data must get through
        # the execute and write stages of core 1
        expectedLatency = self.pipelineLatency + 1
        
        self.localNetworkLatency = self.findLatency("Local network",
                                                    lambda:self.readReg(1,10) == 1337,
                                                    offset=expectedLatency)
    
    def findMemoryLatency(self):
        # When loading a value from memory, the load instruction must pass through
        # the pipeline, the request must cross the network, the data must be loaded,
        # the data must come back, and get through the final two pipeline stages.
        expectedLatency = self.pipelineLatency + 2*self.localNetworkLatency + 1
        
        self.execute(0, "ldw r0 0x2000 -> 1")
        self.execute(0, "or.eop r10 ch0 r0")
        self.cacheMissLatency = self.findLatency("Cache miss",
                                                 lambda:self.readReg(0,10) == 1000,
                                                 offset = expectedLatency)
        
        self.execute(0, "ldw r0 0x2000 -> 1")
        self.execute(0, "or.eop r11 ch0 r0")
        self.cacheHitLatency = self.findLatency("Cache hit",
                                                lambda:self.readReg(0,11) == 1000,
                                                offset = expectedLatency)
                                                
        print "Delay between load and consume:\t", (self.cacheHitLatency + expectedLatency - 2), "cycles"
    
    def findRouterLatency(self):
        pass
            
if __name__ == '__main__':
    CollectStats().runAllTests()
