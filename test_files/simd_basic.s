# SIMD framework: insert this code before the main program, and the core to
# access it will become the control core, and distribute the work across the
# given number of SIMD cores.
#
# Assumes that the number of cores to use is in memory 13 at address 0.
#
# r30 and r31 hold the SIMD parameters---the core's position in the SIMD vector
# and the total number of members, respectively---so probably should not be
# overwritten by the SIMD program.

_start:
    ldw                 0(r0)           -> 1    # load number of SIMD members
#    fetch               r0,  setuploop

    or                  r10, ch0, r0            # r10 = number of SIMD members
    addui               r7,  r10, -1            # r7 = current core we're sending to
    ori                 r8,  r0,  (0,1,0)       # r8 = input ports per core
    mullw               r5,  r7,  r8            # r5 = remote core's instruction input
    addui               r6,  r5,  (0,0,7)       # r6 = remote core's last data input
    fetch.eop           r0,  setuploop

# Set up connections to remote core
setuploop:
    setchmap            2,   r5                 # instruction input = map 2
    setchmap            3,   r6                 # data input = map 3

# Send core any parameters it needs
    ori                 r0,  r7,  0    -> 3     # send core its ID
    ori                 r0,  r3,  0    -> 3     # send core cache configuration
    ori                 r0,  r10, 0    -> 3     # send core number of members

# Set up remote core's channel map table and fetch program.
# Note that we need the IPK FIFO to be at least as long as this code sequence.
    rmtexecute                         -> 2
    ifp?or              r30, ch5, r0            # r30 = this core's SIMD ID
    ifp?ori             r5,  r0,  (0,0,1,0,0)
    ifp?mullw           r6,  r5,  r30
    ifp?slli            r6,  r6,  1             # claim pairs of ports => multiply by 2
    ifp?addu            r3,  ch5, r6            # instruction memory connection
    ifp?addu            r4,  r3,  r5            # data memory connection
    ifp?ori             r8,  r0,  (0,1,0)       # number of ports per core
    ifp?mullw           r8,  r8,  r30           # r8 = first local port
    ifp?setchmap        0,   r3
    ifp?ori             r0,  r0,  0x01000000 -> 0  # tell memory we're about to connect
    ifp?addui           r0,  r8,  (0,0,1) -> 0  # connect to the instruction memory
    ifp?setchmap        1,   r4
    ifp?ori             r0,  r0,  0x01000000 -> 1  # tell memory we're about to connect
    ifp?addui           r0,  r8,  (0,0,2) -> 1  # connect to the data memory
    ifp?or              r31, ch5, r0            # r31 = number of SIMD members
    ifp?or              r0,  ch0, r0            # consume memory's sync message
    ifp?fetch           r0,  4384               # load program (hardcoded address = bad)

# If more members, loop
    addui               r7,  r7,  -1            # update to next member
    setgte.p            r0,  r7,  r0            # see if we have started all members
    ifp?subu            r5,  r5,  r8            # update instruction input
    ifp?subu            r6,  r6,  r8            # update data input
    ifp?ibjmp           -216                    # loop if there's another member
    or.eop              r0,  r0,  r0
