_start:
    fetch           r0,  params
    ori             r5,  r0,  (13,0)
    ori             r6,  r0,  (14,0)
    setchmap        1,   r5                 # input data memory = map 1
    setchmap        2,   r6                 # output data memory = map 2
    ori             r0,  r0,  (0,2)   > 1   # connect to the input data memory
    ori.eop         r0,  r0,  (0,3)   > 2   # connect to the output data memory

params:
    fetch           r0,  loop               # get the next instruction packet

# Load the parameters for this filter. (May deadlock if buffers are small?)
    ldw             r0,  4            > 1
    ldw             r0,  8            > 1
    ldw             r0,  12           > 1
    ldw             r0,  16           > 1
    ldw             r0,  20           > 1

# Store the parameters locally.
    ori             r10, ch0, 0             # r10 = number of taps
    ori             r11, ch0, 0             # r11 = location of taps
    ori             r12, ch0, 0             # r12 = length of input
    ori             r13, ch0, 0             # r13 = location of input
    ori             r14, ch0, 0             # r14 = location to write output

# Compute end point (start point + 4*(input length + num taps - 1))
    addui           r26, r10, -1
    slli            r26, r26, 2             # r26 = (taps - 1) * 4
    slli            r12, r12, 2             # r12 = input length * 4
    addu            r29, r13, r12           # r29 = input pos + input length * 4
    addu            r27, r29, r26
    addui           r27, r27, -4            # r27 = end point
    addu            r28, r11, r26           # r28 = end of taps

    ori.eop         r4,  r13, 0             # r4 = position in outer loop

# Start of outer loop
loop:
    ori             r9,  r0,  0             # r9 = accumulated result
    subu            r6,  r4,  r26           # r6 = current input element
    ori             r5,  r11, 0             # r5 = current tap we're using

# Start of inner loop
    setgteu.p       r0,  r6,  r13           # make sure we are beyond start of input
    ifp?setltu.p    r0,  r6,  r29           # make sure we are before end of input
    ifp?ldw         r6,  0            > 1   # load input data
    ifp?ldw         r5,  0            > 1   # load tap
    addui           r5,  r5,  4             # move to next tap (+ hide memory latency)
    ifp?ori         r7,  ch0, 0
    ifp?mullw       r7,  ch0, r7
    ifp?addu        r9,  r9,  r7            # add new value to result

    setgteu.p       r0,  r28, r5            # see if we have gone through all taps yet
    addui           r6,  r6,  4             # move to next input element
    ifp?ibjmp       -80
# End of inner loop

    addui           r4,  r4,  4             # increment position in outer loop
    stw             r9,  r14, 0       > 2   # store the result
    setgteu.p       r0,  r27, r4            # see if we have finished the outer loop
    addui           r14, r14, 4             # update store location for next time
    ifp?ibjmp       -144
# End of outer loop

    syscall.eop     1
