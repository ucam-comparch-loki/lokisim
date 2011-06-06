# RGB to YUV (technically YCbCr) colour space conversion
#
# Reads RGB data from three separate arrays, and outputs YUV data to three
# separate arrays. Each value is one byte long.
#
#   Y = (( 66R + 129G +  25B + 128) >> 8) + 16
#   U = ((-38R -  74G + 112B + 128) >> 8) + 128
#   V = ((112R -  94G -  18B + 128) >> 8) + 128

# Load parameters
_start:
#    fetch               r0,  load
    ldw                 r0,  4           -> 1
    ldw                 r0,  8           -> 1
    ori                 r2,  ch0, 0             # r2 = rows of input
    ori                 r3,  ch0, 0             # r3 = columns of input
    mullw               r4,  r3,  r2            # r4 = length of input

# Initialisation
    ori                 r10, r0,  0             # r10 = current location
    fetch.eop           r0,  load

# Start of loop
# Load subpixel values
load:
#    fetch               r0,  computey
    ldbu                r10, 0x10000     -> 1   # load red
    ldbu                r10, 0x11000     -> 1   # load green
    ldbu                r10, 0x12000     -> 1   # load blue
    ori                 r7,  ch0, 0             # r7 = red
    ori                 r8,  ch0, 0             # r8 = green
    ori                 r9,  ch0, 0             # r9 = blue
    fetch.eop           r0,  computey

# Compute Y
computey:
#    fetch               r0,  compute
    ori                 r25, r0,  66            # store coefficient
    ori                 r26, r0,  129           # store coefficient
    ori                 r27, r0,  25            # store coefficient
    addui               r29, r0,  computeu      # store next IPK location
    fetch.eop           r0,  compute

# Store Y, compute U
computeu:
    addui               r25, r25, 16            # r25 = Y
    stb                 r25, r10, 0x20000 -> 1  # store Y
#    fetch               r0,  compute
    ori                 r25, r0,  -38           # store coefficient
    ori                 r26, r0,  -74           # store coefficient
    ori                 r27, r0,  112           # store coefficient
    addui               r29, r0,  computev      # store next IPK location
    fetch.eop           r0,  compute

# Store U, compute V
computev:
    addui               r25, r25, 128           # r25 = U
    stb                 r25, r10, 0x21000 -> 1  # store U
#    fetch               r0,  compute
    ori                 r25, r0,  112           # store coefficient
    ori                 r26, r0,  -94           # store coefficient
    ori                 r27, r0,  -18           # store coefficient
    addui               r29, r0,  endofloop     # store next IPK location
    fetch.eop           r0,  compute

# Store V, loop if necessary
endofloop:
    addui               r25, r25, 128           # r25 = V
    stb                 r25, r10, 0x22000 -> 1  # store V
    addui               r10, r10, 1
    setlt.p             r0,  r10, r4            # see if we have finished
    ori                 r25, r0,  load
    ori                 r26, r0,  exit
    psel.fetch.eop      r25, r26

exit:
    syscall.eop         1

# Subroutine to compute (xR + yG + zB + 128) >> 8
# Excludes final addition because it is more efficiently done with addui.
# Assumes x is in r25, y is in r26, z is in r27 and next IPK address is in r29.
# Leaves result in r25.
compute:
    mullw               r25, r25, r7            # xR
    mullw               r26, r26, r8            # yG
    mullw               r27, r27, r9            # zB
#    fetch               r29, 0
    addu                r25, r25, r26           # xR + yG
    addu                r25, r25, r27           # xR + yG + zB
    addui               r25, r25, 128           # xR + yG + zB + 128
    srli                r25, r25, 8             # (xR + yG + zB + 128) >> 8
    fetch.eop           r29, 0
