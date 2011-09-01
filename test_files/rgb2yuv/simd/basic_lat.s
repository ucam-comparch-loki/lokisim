# RGB to YUV (technically YCbCr) colour space conversion
#
# Reads RGB data from three separate arrays, and outputs YUV data to three
# separate arrays. Each value is one byte long.
#
#   Y = (( 66R + 129G +  25B + 128) >> 8) + 16
#   U = ((-38R -  74G + 112B + 128) >> 8) + 128
#   V = ((112R -  94G -  18B + 128) >> 8) + 128

# Load parameters
simdstart:
    ldw                 4(r0)            -> 1
    ldw                 8(r0)            -> 1
    fetch               r0,  load
    ori                 r2,  ch0, 0             # r2 = rows of input
    ori                 r3,  ch0, 0             # r3 = columns of input
    mullw               r4,  r3,  r2            # r4 = length of input

# Initialisation
    or                  r10, r0,  r30           # r10 = current location
    or                  r11, r0,  r30           # r11 = current store location
    ldbu                0x10000(r10)     -> 1   # load red
    ldbu                0x11000(r10)     -> 1   # load green
    ldbu                0x12000(r10)     -> 1   # load blue
    addu.eop            r10, r10, r31           # move to next pixel

# Start of loop
# Load subpixel values
load:
    ldbu                0x10000(r10)     -> 1   # load red
    ldbu                0x11000(r10)     -> 1   # load green
    ldbu                0x12000(r10)     -> 1   # load blue
    fetch               r0,  computey
    addu                r10, r10, r31           # move to next pixel
    ori                 r7,  ch0, 0             # r7 = red
    ori                 r8,  ch0, 0             # r8 = green
    ori.eop             r9,  ch0, 0             # r9 = blue

# Compute Y
computey:
    fetch               r0,  compute
    ori                 r13, r0,  66            # store coefficient
    ori                 r14, r0,  129           # store coefficient
    ori                 r15, r0,  25            # store coefficient
    addui.eop           r29, r0,  computeu      # store next IPK location

# Store Y, compute U
computeu:
    addui               r13, r13, 16            # r13 = Y
    stb                 r13, 0x20000(r11) -> 1  # store Y
    fetch               r0,  compute
    ori                 r13, r0,  -38           # store coefficient
    ori                 r14, r0,  -74           # store coefficient
    ori                 r15, r0,  112           # store coefficient
    addui.eop           r29, r0,  computev      # store next IPK location

# Store U, compute V
computev:
    addui               r13, r13, 128           # r13 = U
    stb                 r13, 0x21000(r11) -> 1  # store U
    fetch               r0,  compute
    ori                 r13, r0,  112           # store coefficient
    ori                 r14, r0,  -94           # store coefficient
    ori                 r15, r0,  -18           # store coefficient
    addui.eop           r29, r0,  endofloop     # store next IPK location

# Store V, loop if necessary
endofloop:
    addui               r13, r13, 128           # r13 = V
    stb                 r13, 0x22000(r11) -> 1  # store V
    addu                r11, r11, r31           # move to next pixel
    setlt.p             r0,  r11, r4            # see if we have finished
    ori                 r25, r0,  load
    ori                 r26, r0,  exit
    psel.fetch.eop      r25, r26

exit:
    seteq.p             r0,  r30, r0            # set p if we are core 0
    ifp?syscall         1
    or.eop              r0,  r0,  r0

# Subroutine to compute (xR + yG + zB + 128) >> 8
# Excludes final addition because I currently don't have enough registers for it.
# Assumes x is in r13, y is in r14, z is in r15 and next IPK address is in r29.
# Leaves result in r13.
compute:
    fetch               r29, 0
    mullw               r13, r13, r7            # xR
    mullw               r14, r14, r8            # yG
    mullw               r15, r15, r9            # zB
    addu                r13, r13, r14           # xR + yG
    addu                r13, r13, r15           # xR + yG + zB
    addui               r13, r13, 128           # xR + yG + zB + 128
    srli.eop            r13, r13, 8             # (xR + yG + zB + 128) >> 8
