# All of the persistent instruction packets to be executed by each core.
#
# It is assumed that each core already has two connections to memory:
#   Output channel 0 for instructions
#   Output channel 1 for data (received on input ch5)
#
# If the core needs to send its results to another core, it is assumed that
# this is set up on output channel 2.

mul_constant:                                   # Address 4096
    mullw.eop       r0,  ch0, r10       -> 2

add:                                            # Address 4104
    addu.eop        r0,  ch0, ch1       -> 2

add_16:                                         # Address 4112
    addui.eop       r0,  ch0, 16        -> 2

add_128:                                        # Address 4120
    addui.eop       r0,  ch0, 128       -> 2

shift_8:                                        # Address 4128
    srli.eop        r0,  ch0, 8         -> 2

store_y:                                        # Address 4136
    stb.eop         ch0, ch1, 0x20000   -> 1

store_u:                                        # Address 4144
    stb.eop         ch0, ch1, 0x21000   -> 1

store_v:                                        # Address 4152
    stb.eop         ch0, ch1, 0x22000   -> 1

# Plenty of scope for optimising loading of values (e.g. loop unrolling).
# If memories can send data to cores other than the requester, this critical-
# path dependency would disappear.
load_r:                                         # Address 4160
    ldbu            ch0, 0x10000        -> 1
    or.eop          r0,  ch5, r0        -> 2

load_g:                                         # Address 4176
    ldbu            ch0, 0x11000        -> 1
    or.eop          r0,  ch5, r0        -> 2

load_b:                                         # Address 4192
    ldbu            ch0, 0x12000        -> 1
    or.eop          r0,  ch5, r0        -> 2

# Generate a sequence of numbers from 0 to the size of the input.
# Could unroll loop if this becomes a bottleneck.
address_gen:                                    # Address 4208
    ldw             r0,  4              -> 1
    ldw             r0,  8              -> 1
    or              r2,  ch5, r0                # receive rows of input
    mullw           r2,  ch5, r2                # receive columns of input
    addui           r2,  r2,  -1                # r2 = length of input - 1
    or              r5,  r0,  r0        -> 2    # r5 = position in input

    setlt.p         r0,  r5,  r2                # See if we have reached the end of data
    ifp?addui       r5,  r5,  1         -> 2
    ifp?ibjmp       -16
    or.eop          r0,  r0,  r0                # should someone syscall here?
