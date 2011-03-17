# SIMD framework: insert this code before the main program, and the core to
# access it will become the control core, and distribute the work across the
# given number of SIMD cores.
#
# Assumes that the number of cores to use is in memory 13 at address 0.
#
# r30 and r31 hold the SIMD parameters---the core's position in the SIMD vector
# and the total number of members, respectively---so probably should not be
# overwritten by the SIMD program.

# Set up connection to memory containing arguments
_start:
    ori                 r4,  r0,  (9,0)
    setchmap            1,   r4
    ori                 r0,  r0,  (0,2) -> 1    # connect to the argument memory
    ldw                 r0,  0          -> 1    # load number of SIMD members
    fetch               r0,  setuploop

    or                  r10, ch0, r0            # r10 = number of SIMD members
    addui               r7,  r10, -1            # r7 = current core we're sending to
    ori                 r8,  r0,  (1,0)         # r8 = input ports per core
    mullw               r5,  r7,  r8            # r5 = remote core's instruction input
    addui.eop           r6,  r5,  2             # r6 = remote core's data input

# Set up connections to remote core
setuploop:
    setchmap            2,   r5                 # instruction input = map 2
    setchmap            3,   r6                 # data input = map 3

# Send core any parameters it needs
    ori                 r0,  r7,  0    -> 3     # send core its ID
    ori                 r0,  r10, 0,   -> 3     # send core number of members

# Set up remote core's channel map table and fetch program.
# Note that we need the IPK FIFO to be at least as long as this code sequence.
    rmtexecute                         -> 2
    p?ori               r30, ch0, 0             # r30 = this core's SIMD ID
    p?addui             r4,  r30, (8,0)         # compute memory port to use
    p?ori               r8,  r0,  (1,0)         # number of ports per core
    p?mullw             r8,  r8,  r30           # r8 = first local port
    p?setchmap          0,   r4
    p?addui             r0,  r8,  1    -> 0     # connect to the instruction memory
    p?ori               r31, ch0, 0             # r31 = number of SIMD members
    p?fetch             r0,  4328               # load program (hardcoded address = bad)

# If more members, loop
    addui               r7,  r7,  -1            # update to next member
    setgte.p            r0,  r7,  r0            # see if we have started all members
    p?subu              r5,  r5,  r8            # update instruction input
    p?subu              r6,  r6,  r8            # update data input
    p?ibjmp             -136                    # loop if there's another member
    or.eop              r0,  r0,  r0
