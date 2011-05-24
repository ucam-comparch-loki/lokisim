# Very similar to default code, but makes use of indirect register file to
# cache the taps. We can have no more than 32 taps.

# Set up connections to memories (instruction memory is already sorted).
simdstart:
    fetch           r0,  params             # fetch main program
    addui           r5,  r30, (9,0)
    addui           r6,  r30, (10,0)
    setchmap        1,   r5                 # input data memory = map 1
    setchmap        2,   r6                 # output data memory = map 2
    addui           r0,  r8,  2   > 1       # connect to the input data memory
    addui.eop       r0,  r8,  3   > 2       # connect to the output data memory

params:
    fetch           r0,  loadtaps           # get the next instruction packet

# Load the parameters for this filter. (May deadlock if buffers are small?)
    ldw             r0,  4        > 1
    ldw             r0,  8        > 1
    ldw             r0,  12       > 1
    ldw             r0,  16       > 1
    ldw             r0,  20       > 1

    slli            r2,  r30, 2             # initial position is function of SIMD ID
    slli            r3,  r31, 2             # r3 = stride length

# Store the parameters locally.
    ori             r10, ch0, 0             # r10 = number of taps
    ori             r11, ch0, 0             # r11 = location of taps
    ori             r12, ch0, 0             # r12 = length of input
    ori             r13, ch0, 0             # r13 = location of input
    addu            r14, ch0, r2            # r14 = location to write output

# Compute end point (start point + 4*(input length + num taps - 1))
    addui           r26, r10, -1
    slli            r26, r26, 2             # r26 = (taps - 1) * 4
    slli            r12, r12, 2             # r12 = input length * 4
    addu            r29, r13, r12           # r29 = input pos + input length * 4
    addu            r27, r29, r26
    addui           r27, r27, -4            # r27 = end point
    addu            r28, r11, r26           # r28 = end of taps

    addu.eop        r4,  r13, r2            # r4 = position in outer loop

# Load taps into indirect register file.
loadtaps:
    fetch           r0,  loop
    ori             r5,  r0,  32            # r5 = pointer to register to store tap in
    ldw             r11, 0          > 1     # load a tap
    addui           r11, r11, 4             # move to next tap
    iwtr            r5,  ch0                # store the tap in the IRF
    addui           r5,  r5,  1             # move to next register
    setgteu.p       r0,  r28, r11           # see if we have gone through all taps yet
    p?ibjmp         -40                     # if not, load another tap
    addui.eop       r8,  r10, 32            # r8 = first register with no tap in it

# Start of outer loop
loop:
    ori             r9,  r0,  0             # r9 = accumulated result
    subu            r6,  r4,  r26           # r6 = current input element
    ori             r5,  r0,  32            # move back to start of taps

# Start of inner loop
    setgteu.p       r0,  r6,  r13           # make sure we are beyond start of input
    p?setltu.p      r0,  r6,  r29           # make sure we are before end of input
    p?ldw           r6,  0          > 1     # load input data
    p?irdr          r7,  r5                 # load tap
    addui           r5,  r5,  1             # move to next tap
    p?mullw         r7,  ch0, r7
    p?addu          r9,  r9,  r7            # add new value to result

    setlt.p         r0,  r5,  r8            # see if we have gone through all taps yet
    addui           r6,  r6,  4             # move to next input element
    p?ibjmp         -72
# End of inner loop

    addu            r4,  r4,  r3            # increment position in outer loop
    stw             r9,  r14, 0     > 2     # store the result
    setgteu.p       r0,  r27, r4            # see if we have finished the outer loop
    addu            r14, r14, r3            # update store location for next time
    p?ibjmp         -136
# End of outer loop

    seteqi.p        r0,  r30, 0             # only exit if we're the last core
    ifp?syscall     1
    or.eop          r0,  r0,  r0