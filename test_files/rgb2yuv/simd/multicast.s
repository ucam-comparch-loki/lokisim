# RGB to YUV (technically YCbCr) colour space conversion
#
# Reads RGB data from three separate arrays, and outputs YUV data to three
# separate arrays. Each value is one byte long.
#
#   Y = (( 66R + 129G +  25B + 128) >> 8) + 16
#   U = ((-38R -  74G + 112B + 128) >> 8) + 128
#   V = ((112R -  94G -  18B + 128) >> 8) + 128

# So that absolute positions can be used in the SIMD setup code, have a constant
# size packet here which just fetches the main helper program.
hackystart:
    fetch.eop               r0,  helpercode

# Load parameters
simdstart:
#    fetch               r0,  load

# Initialisation
    addui               r10, r30, -1            # r10 = current location
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
    fetch.eop           r0,  compute

# Store Y, compute U
computeu:
    stb                 r13, r10, 0x20000 -> 1  # store Y
    fetch.eop           r0,  compute

# Store U, compute V
computev:
    stb                 r13, r10, 0x21000 -> 1  # store U
    fetch.eop           r0,  compute

# Store V, loop if necessary
endofloop:
    stb                 r13, r10, 0x22000 -> 1  # store V
    setne.p             r0,  ch5, r0            # see if we have finished
    addu                r10, r10, r31           # move to next pixel
    ifp?fetch           r0,  load
    if!p?fetch          r0,  exit
    or.eop              r0,  r0,  r0

exit:
    seteqi.p            r0,  r30, 1             # set p if we are core 1
    ifp?syscall         1
    or.eop              r0,  r0,  r0

# Subroutine to compute (xR + yG + zB + 128) >> 8
# Leaves result in r13.
compute:
    mullw               r13, ch5, r7            # xR
    mullw               r14, ch5, r8            # yG
    mullw               r15, ch5, r9            # zB
#    fetch               ch5, 0
    addu                r13, r13, r14           # xR + yG
    addu                r13, r13, r15           # xR + yG + zB
    addui               r13, r13, 128           # xR + yG + zB + 128
    srli                r13, r13, 8             # (xR + yG + zB + 128) >> 8
    addu                r13, r13, ch5           # completed computation
    fetch.eop           ch5, 0



helpercode:
#    fetch               r0,  helpcomputey
    ldw                 r0,  4           -> 1
    ldw                 r0,  8           -> 1
    ori                 r2,  ch0, 0             # r2 = rows of input
    ori                 r3,  ch0, 0             # r3 = columns of input
    mullw               r4,  r3,  r2            # r4 = length of input

# Initialisation
    or                  r10, r0,  r0            # r10 = current location
    fetch.eop           r0,  helpcomputey

# Start of loop

# Compute Y
helpcomputey:
#    fetch               r0,  helpcomputeu
    ori                 r0,  r0,  66     -> 3   # send coefficient
    ori                 r0,  r0,  129    -> 3   # send coefficient
    ori                 r0,  r0,  25     -> 3   # send coefficient
    ori                 r0,  r0,  16     -> 3   # send coefficient
    ori                 r0,  r0,  computeu -> 3   # send coefficient
    fetch.eop           r0,  helpcomputeu

# Store Y, compute U
helpcomputeu:
#    fetch               r0,  helpcomputev
    ori                 r0,  r0,  -38    -> 3   # send coefficient
    ori                 r0,  r0,  -74    -> 3   # send coefficient
    ori                 r0,  r0,  112    -> 3   # send coefficient
    ori                 r0,  r0,  128    -> 3   # send coefficient
    ori                 r0,  r0,  computev -> 3   # send coefficient
    fetch.eop           r0,  helpcomputev

# Store U, compute V
helpcomputev:
#    fetch               r0,  helpendofloop
    ori                 r0,  r0,  112    -> 3   # send coefficient
    ori                 r0,  r0,  -94    -> 3   # send coefficient
    ori                 r0,  r0,  -18    -> 3   # send coefficient
    ori                 r0,  r0,  128    -> 3   # send coefficient
    ori                 r0,  r0,  endofloop -> 3   # send coefficient
    fetch.eop           r0,  helpendofloop

# Store V, loop if necessary
helpendofloop:
    addu                r10, r10, r31           # move to next pixel
    setlt.p             r0,  r10, r4     -> 3   # see if we have finished
    ifp?fetch           r0,  helpcomputey
    if!p?fetch          r0,  exit
    or.eop              r0,  r0,  r0
