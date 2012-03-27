# Very similar to default code, but makes use of indirect register file to
# cache the taps. We can have no more than 32 taps.

simdstart:

# Load the parameters for this filter. (May deadlock if buffers are small?)
# Some of the channel reads also have to go here to prevent the data being
# reordered. This should be fixed eventually.
    ldw             4(r0)        -> 1
    addu            r10, ch0, r0            # r10 = number of taps
    ldw             8(r0)        -> 1
    addu            r11, ch0, r0            # r11 = location of taps
    ldw             12(r0)       -> 1
    addu            r12, ch0, r0            # r12 = length of input
    ldw             16(r0)       -> 1
    addu            r13, ch0, r0            # r13 = location of input
    ldw             20(r0)       -> 1

    slli            r16, r30, 2             # initial position is function of SIMD ID
    slli            r17, r31, 2             # r3 = stride length

    addu            r14, ch0, r16           # r14 = location to write output

    fetchr          loadtaps                # get the next instruction packet

# Compute end point (start point + 4*(input length + num taps - 1))
    addui           r26, r10, -1
    slli            r26, r26, 2             # r26 = (taps - 1) * 4
    slli            r12, r12, 2             # r12 = input length * 4
    addu            r29, r13, r12           # r29 = input pos + input length * 4
    addu            r27, r29, r26
    addui           r27, r27, -4            # r27 = end point
    addu            r28, r11, r26           # r28 = end of taps

    addu.eop        r18, r13, r16           # r18 = position in outer loop

# Load taps into indirect register file.
loadtaps:
    fetchr          loop
    addui           r19, r0,  32            # r19 = pointer to register to store tap in
    ldw             0(r11)         -> 1     # load a tap (early)
    addui           r11, r11, 4             # move to next tap

    ldw             0(r11)         -> 1     # load a tap
    addui           r11, r11, 4             # move to next tap
    iwtr            r19, ch0                # store the tap in the IRF
    addui           r19, r19, 1             # move to next register
    setgteu.p       r0,  r28, r11           # see if we have gone through all taps yet
    ifp?ibjmp       -20                     # if not, load another tap

    iwtr            r19, ch0                # store the tap in the IRF (late)
    addui.eop       r22, r10, 32            # r22 = first register with no tap in it

# Start of outer loop
loop:
    addu            r23, r0,  r0            # r23 = accumulated result
    subu            r20, r18, r26           # r20 = current input element
    ldw             0(r20)         -> 1     # load input data
    addui           r19, r0,  32            # move back to start of taps

# Start of inner loop
    setgteu.p       r0,  r20, r13           # make sure we are beyond start of input
    ifp?setltu.p    r0,  r20, r29           # make sure we are before end of input
    addui           r20, r20, 4             # move to next input element
    ifp?irdr        r21, r19                # load tap
    addui           r19, r19, 1             # move to next tap
    mullw           r21, ch0, r21
    ifp?addu        r23, r23, r21           # add new value to result

    setlt.p         r0,  r19, r22           # see if we have gone through all taps yet
    ifp?ldw         0(r20)         -> 1     # load input data
    ifp?ibjmp       -36
# End of inner loop

    addu            r18, r18, r17           # increment position in outer loop
    stw             r23, 0(r14)    -> 1     # store the result
    setgteu.p       r0,  r27, r18           # see if we have finished the outer loop
    addu            r14, r14, r17           # update store location for next time
    ifp?ibjmp       -72
# End of outer loop

    #seteq.p         r0,  r30, r0            # set p if we are core 0
    #ifp?syscall     1
    #addu.eop        r0,  r0,  r0

    addui               r31, r31, -1            # num cores -> highest core index
    setlt.p             r0,  r30, r31           # all cores except the highest receive word
    ifp?addu            r0,  ch4, r0            # receive word

    seteq.p             r0,  r30, r0            # all cores except 0 send a word
    if!p?lli            r10, (0,1,0)            # start computing address of prev core
    if!p?lli            r11, (0,0,6)            # want to send to input 6 (ch4)
    if!p?addui          r12, r30, -1            # previous core id
    if!p?mullw          r10, r10, r12           # now have something like (0,4,0)
    if!p?addu           r10, r10, r11           # now have something like (0,4,6)
    if!p?setchmapi      4,   r10
    if!p?addu           r0,  r0,  r0    -> 4    # send sync message

    ifp?syscall         1                       # if we are core 0, end
    addu.eop            r0,  r0,  r0
