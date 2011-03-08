# RGB to YUV (technically YCbCr) colour space conversion
#
# Reads RGB data from three separate arrays, and outputs YUV data to three
# separate arrays. Each value is one byte long.
#
#   Y = (( 66R + 129G +  25B + 128) >> 8) + 16
#   U = ((-38R -  74G + 112B + 128) >> 8) + 128
#   V = ((112R -  94G -  18B + 128) >> 8) + 128

# Set up connections to memories
_start:
    fetch               r0,  params
    ori                 r5,  r0,  (9,0)
    ori                 r6,  r0,  (10,0)
    setchmap            1,   r5                 # input = map 1
    setchmap            2,   r6                 # output = map 2
    ori                 r0,  r0,  (0,2)  -> 1
    ori.eop             r0,  r0,  (0,0)  -> 2

# Load parameters
params:
    fetch               r0,  load
    ldw                 r0,  4           -> 1
    ldw                 r0,  8           -> 1
    ori                 r2,  ch0, 0             # r2 = rows of input
    ori                 r3,  ch0, 0             # r3 = columns of input
    mullw               r4,  r3,  r2            # r4 = length of input

# Initialisation
    ori                 r5,  r4,  0             # r5 = start of second input (green)
    addu                r6,  r5,  r5            # r6 = start of third input (blue)
    ori.eop             r10, r0,  0             # r10 = current location

# Start of loop
# Load subpixel values
load:
    fetch               r0,  computey
    ldbu                r10, 12          -> 1   # load red (offset = length of params)
    addu                r11, r5,  r10           # r11 = position in green array
    ldbu                r11, 12          -> 1   # load green
    addu                r12, r6,  r10           # r12 = position in blue array
    ldbu                r12, 12          -> 1   # load blue
    ori                 r7,  ch0, 0             # r7 = red
    ori                 r8,  ch0, 0             # r8 = green
    ori.eop             r9,  ch0, 0             # r9 = blue

# Compute Y
computey:
    fetch               r0,  compute
    ori                 r20, r0,  66            # store coefficient
    ori                 r21, r0,  129           # store coefficient
    ori                 r22, r0,  25            # store coefficient
    addui.eop           r29, r0,  computeu      # store next IPK location

# Store Y, compute U
computeu:
    addui               r20, r20, 16            # r20 = Y
    stb                 r20, r10, 0      -> 2   # store Y
    fetch               r0,  compute
    ori                 r20, r0,  -38           # store coefficient
    ori                 r21, r0,  -74           # store coefficient
    ori                 r22, r0,  112           # store coefficient
    addui.eop           r29, r0,  computev      # store next IPK location

# Store U, compute V
computev:
    addui               r20, r20, 128           # r20 = U
    stb                 r20, r11, 0      -> 2   # store U
    fetch               r0,  compute
    ori                 r20, r0,  112           # store coefficient
    ori                 r21, r0,  -94           # store coefficient
    ori                 r22, r0,  -18           # store coefficient
    addui.eop           r29, r0,  endofloop     # store next IPK location

# Store V, loop if necessary
endofloop:
    addui               r20, r20, 128           # r20 = V
    stb                 r20, r12, 0      -> 2   # store V
    addui               r10, r10, 1
    setlt.p             r0,  r10, r4            # see if we have finished
    ifp?fetch           r0,  load
    if!p?fetch          r0,  exit
    or.eop              r0,  r0,  r0

exit:
    syscall.eop         1

# Subroutine to compute (xR + yG + zB + 128) >> 8
# Excludes final addition because it is more efficiently done with addui.
# Assumes x is in r20, y is in r21, z is in r22 and next IPK address is in r29.
# Leaves result in r20.
compute:
    mullw               r20, r20, r7            # xR
    mullw               r21, r21, r8            # yG
    mullw               r22, r22, r9            # zB
    fetch               r29, 0
    addu                r20, r20, r21           # xR + yG
    addu                r20, r20, r22           # xR + yG + zB
    addui               r20, r20, 128           # xR + yG + zB + 128
    srli.eop            r20, r20, 8             # (xR + yG + zB + 128) >> 8
