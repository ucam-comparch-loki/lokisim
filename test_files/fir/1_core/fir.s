_start:

# Load the parameters for this filter. (May deadlock if buffers are small?)
    ldw             4(r0)           -> 1
    ldw             8(r0)           -> 1
    ldw             12(r0)          -> 1
    ldw             16(r0)          -> 1
    ldw             20(r0)          -> 1

    fetchr          loop                    # get the next instruction packet

# Store the parameters locally.
    addu            r10, ch0, r0            # r10 = number of taps
    addu            r11, ch0, r0            # r11 = location of taps
    addu            r12, ch0, r0            # r12 = length of input
    addu            r13, ch0, r0            # r13 = location of input
    addu            r14, ch0, r0            # r14 = location to write output

# Compute end point (start point + 4*(input length + num taps - 1))
    addui           r26, r10, -1
    slli            r26, r26, 2             # r26 = (taps - 1) * 4
    slli            r12, r12, 2             # r12 = input length * 4
    addu            r29, r13, r12           # r29 = input pos + input length * 4
    addu            r27, r29, r26
    addui           r27, r27, -4            # r27 = end point
    addu            r28, r11, r26           # r28 = end of taps

    addu.eop        r16, r13, r0            # r16 = position in outer loop

# Start of outer loop
loop:
    addu            r20, r0,  r0            # r20 = accumulated result
    subu            r18, r16, r26           # r18 = current input element
    addu            r17, r11, r0            # r17 = current tap we're using

# Start of inner loop
    setgteu.p       r0,  r18, r13           # make sure we are beyond start of input
    ifp?setltu.p    r0,  r18, r29           # make sure we are before end of input
    ifp?ldw         0(r18)           -> 1   # load input data
    ifp?ldw         0(r17)           -> 1   # load tap
    addui           r17, r17, 4             # move to next tap (+ hide memory latency)
    ifp?addu        r19, ch0, r0
    ifp?mullw       r19, ch0, r19
    ifp?addu        r20, r20, r19           # add new value to result

    setgteu.p       r0,  r28, r17           # see if we have gone through all taps yet
    addui           r18, r18, 4             # move to next input element
    ifp?ibjmp       -40
# End of inner loop

    addui           r16, r16, 4             # increment position in outer loop
    stw             r20, 0(r14)      -> 1   # store the result
    setgteu.p       r0,  r27, r16           # see if we have finished the outer loop
    addui           r14, r14, 4             # update store location for next time
    ifp?ibjmp       -72
# End of outer loop

    syscall.eop     1
