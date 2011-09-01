# Very similar to default code, but makes use of indirect register file to
# cache the taps. We can have no more than 32 taps.

# So that absolute positions can be used in the SIMD setup code, have a constant
# size packet here which just fetches the main helper program.
hackystart:
    fetch.eop               r0,  helpercode

simdstart:
    fetch           r0,  loop               # get the next instruction packet

    addui           r2,  r30, -1            # core 0 doesn't do work => subtract 1
    slli            r2,  r2,  2             # initial position is function of SIMD ID
    slli            r3,  r31, 2             # r3 = stride length

# Receive parameters from control core.
    or              r13, ch5, r0            # r13 = location of input
    addu            r14, ch5, r2            # r14 = location to write output

# Receive information about where the ends of various arrays are.
    or              r26, ch5, r0            # r26 = end of taps
    or              r25, ch5, r0            # r25 = end of input

    addu.eop        r4,  r13, r2            # r4 = position in outer loop

# Start of outer loop
loop:
    ori             r9,  r0,  0             # r9 = accumulated result
    subu            r6,  r4,  r26           # r6 = current input element
    ldw             0(r6)          -> 1     # load input data

# Start of inner loop
    setgteu.p       r0,  r6,  r13           # make sure we are beyond start of input
    ifp?setltu.p    r0,  r6,  r25           # make sure we are before end of input
    addui           r6,  r6,  4             # move to next input element
    mullw           r7,  ch0, ch5           # multiply data by tap
    ifp?addu        r9,  r9,  r7            # add new value to result

    setne.p         r0,  ch5, r0
    ifp?ldw         0(r6)          -> 1     # load input data
    ifp?ibjmp       -56
# End of inner loop

    addu            r4,  r4,  r3            # increment position in outer loop
    stw             r9,  0(r14)    -> 1     # store the result
    setne.p         r0,  ch5, r0
    addu            r14, r14, r3            # update store location for next time
    ifp?ibjmp       -120
# End of outer loop

    seteqi.p        r0,  r30, 1             # set p if we are core 1
    ifp?syscall     1
    or.eop          r0,  r0,  r0

# Hacky extra instructions to ensure cache-line alignment.
# Stops consecutive loads being reordered when they go to different memories.
or.eop r0, r0, r0
or.eop r0, r0, r0
or.eop r0, r0, r0



# Code to be run by the single helper core.
helpercode:
# Load parameters.
    ldw             4(r0)        -> 1
    ldw             8(r0)        -> 1
    ldw             12(r0)       -> 1
    ldw             16(r0)       -> 1
    ldw             20(r0)       -> 1

    fetch           r0,  helploadtaps

    slli            r3,  r31, 2             # r3 = stride length (in bytes)

# Store the parameters locally (and share with group).
    or              r10, ch0, r0            # r10 = number of taps
    or              r11, ch0, r0            # r11 = location of taps
    or              r12, ch0, r0            # r12 = length of input
    or              r13, ch0, r0 -> 3       # r13 = location of input
    or              r14, ch0, r0 -> 3       # r14 = location to write output

# Compute end point (start point + 4*(input length + num taps - 1))
    addui           r26, r10, -1
    slli            r26, r26, 2  -> 3       # r26 = (taps - 1) * 4
    slli            r12, r12, 2             # r12 = input length * 4
    addu            r25, r13, r12-> 3       # r25 = input pos + input length * 4
    addu            r27, r25, r26
    addui           r27, r27, -4            # r27 = end point
    addu            r28, r11, r26           # r28 = end of taps
    or.eop          r4,  r13, r0            # r4 = current position in input

helploadtaps:
    fetch           r0,  helploop
    ori             r5,  r0,  32            # r5 = pointer to register to store tap in
    ldw             0(r11)         -> 1     # load a tap
    addui           r11, r11, 4             # move to next tap

    ldw             0(r11)         -> 1     # load a tap
    addui           r11, r11, 4             # move to next tap
    setgteu.p       r0,  r28, r11           # see if we have gone through all taps yet
    iwtr            r5,  ch0                # store the tap in the IRF
    addui           r5,  r5,  1             # move to next register
    ifp?ibjmp       -40                     # if not finished, load another tap

    iwtr            r5,  ch0                # store the tap in the IRF
    addui.eop       r8,  r10, 32            # r8 = first register with no tap in it

helploop:
    ori             r5,  r0,  32            # move to start of taps

    irdr            r0,  r5        -> 3     # send current tap
    addui           r5,  r5,  1             # move to next tap
    setlt.p         r0,  r5,  r8   -> 3     # see if we have gone through all taps yet
    ifp?ibjmp       -24

    addu            r4,  r4,  r3
    setgteu.p       r0,  r27, r4   -> 3     # see if we have finished all input
    ifp?ibjmp       -56
    or.eop          r0,  r0,  r0
