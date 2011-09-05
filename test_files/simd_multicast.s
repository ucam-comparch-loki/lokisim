# SIMD framework: insert this code before the main program, and the core to
# access it will become the control core, and distribute the work across the
# given number of SIMD cores.
#
# This version makes use of a single helper core which multicasts data to the
# rest of the group.
#
# Assumes that the number of cores to use is in memory 13 at address 0.
#
# r30 and r31 hold the SIMD parameters---the core's position in the SIMD vector
# and the total number of members, respectively---so probably should not be
# overwritten by the SIMD program. r29 holds a bitmask of the group members.

_start:
    ldw                 0(r0)           -> 1    # load number of SIMD members
    fetch               r0,  send_ids

# Load arguments and prepare to communicate with other cores in the group.
    or                  r31, ch0, r0            # r31 = number of SIMD members
    addui               r7,  r31, -1            # r7 = current core we're sending to
    ori                 r8,  r0,  (0,1,0)       # r8 = input ports per core
    mullw               r5,  r7,  r8            # r5 = remote core's first input
    addui               r6,  r5,  (0,0,7)       # r6 = remote core's last data input
    ori                 r29, r0,  1             # r29 = 1
    sll                 r29, r29, r31           # r29 = 0b100000000
    addui               r29, r29, -2            # r29 = 0b011111110 = bitmask of group members
    ori                 r11, r0,  (m00000000,0) # r11 = multicast address to be built
    addui.eop           r31, r31, -1            # helper core => 1 less group member

# Send any core-specific data to each member of the group, and build up the
# multicast addresses needed.
send_ids:
    fetch               r0,  send_code
    setchmap            3,   r6                 # data input = map 3
    ori                 r0,  r7,  0    -> 3     # send core its ID
    sll                 r5,  r8,  r7            # r5 = multicast bit for this core
    or                  r11, r11, r5            # add the bit to the multicast address
    addui               r7,  r7,  -1            # update to next member
    setgtei.p           r0,  r7,  1             # see if we have sent ID to all members
    ifp?subu            r6,  r6,  r8            # update data input
    ifp?ibjmp           -56                     # loop if there's another member
    addui.eop           r12, r11, (0,0,7)       # another multicast address for data

# Send any data/instructions which can be multicast.
send_code:
    setchmap            2,   r11                # map 2 = all instruction FIFOs
    setchmap            3,   r12                # map 3 = all data inputs

    ori                 r0,  r3,  0    -> 3     # send cores cache configuration
    ori                 r0,  r29, 0    -> 3     # send cores bitmask of group members

# Set up remote cores' channel map tables and fetch program.
# Note that we need the IPK FIFO to be at least as long as this code sequence.
# TODO: turn this into an instruction packet (or two), and put it in one core's
# cache (using fill, rather than fetch). Then have the whole group gfetch it.
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
    ifp?or              r29, ch5, r0            # r29 = bitmask of group members
    ifp?popc            r31, r29                # r31 = number of SIMD members
    ifp?or              r0,  ch0, r0            # consume memory's sync message
    ifp?fetch           r0,  4472               # load program (hardcoded address = bad)

    fetch.eop           r0,  4464               # load helper program
