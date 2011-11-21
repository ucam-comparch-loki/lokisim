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
    fetchr              setuploop

    addu                r31, ch0, r0            # r31 = number of SIMD members
    addui               r21, r31, -1            # r21 = current core we're sending to
    lli                 r22, (0,1,0)            # r22 = input ports per core
    mullw               r19, r21, r22           # r19 = remote core's instruction input
    lli                 r23, (0,0,7)            # final input port of a core
    addu.eop            r23, r19, r23           # r23 = remote core's last data input

# Set up connections to remote core
setuploop:
    setchmapi           2,   r19                # instruction input = map 2
    setchmapi           3,   r23                # data input = map 3

# Send core any parameters it needs
    addu                r0,  r21, r0   -> 3     # send core its ID
    addu                r0,  r20, r0   -> 3     # send core cache configuration
    addu                r0,  r31, r0   -> 3     # send core number of members

# Set up remote core's channel map table and fetch program.
# Note that we need the IPK FIFO to be at least as long as this code sequence.
    rmtexecute                         -> 2
    ifp?addu            r30, ch5, r0            # r30 = this core's SIMD ID
    ifp?lli             r19, (0,0,1,0,0)
    ifp?mullw           r23, r19, r30
    ifp?slli            r23, r23, 1             # claim pairs of ports => multiply by 2
    ifp?addu            r17, ch5, r23           # instruction memory connection
    ifp?addu            r18, r17, r19           # data memory connection
    ifp?lli             r22, (0,1,0)            # number of ports per core
    ifp?mullw           r22, r22, r30           # r22 = first local port
    ifp?setchmapi       0,   r17
    ifp?lui             r8,  0x0100
    ifp?addu            r0,  r8,  r0   -> 0     # tell memory we're about to connect
    ifp?addu            r22, r22, r19  -> 0     # connect to the instruction memory
    ifp?setchmapi       1,   r18
    ifp?addu            r0,  r8,  r0   -> 1     # tell memory we're about to connect
    ifp?addu            r22, r22, r19  -> 1     # connect to the data memory
    ifp?addu            r31, ch5, r0            # r31 = number of SIMD members
    ifp?addu            r0,  ch0, r0            # consume memory's sync message
    ifp?lli             r8,  4252
    ifp?fetch           r8                      # load program (hardcoded address = bad)

# If more members, loop
    addui               r21, r21, -1            # update to next member
    setgte.p            r0,  r21, r0            # see if we have started all members
    ifp?subu            r19, r19, r22           # update instruction input
    ifp?subu            r23, r23, r22           # update data input
    ifp?ibjmp           -116                    # loop if there's another member
    addu.eop            r0,  r0,  r0
