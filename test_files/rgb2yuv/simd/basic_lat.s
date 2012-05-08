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
    fetchr              load
    lli                 r20, %lo(0x10000)       # r20 = start of input
    lui                 r20, %hi(0x10000)
    lli                 r21, %lo(0x20000)       # r21 = start of output
    lui                 r21, %hi(0x20000)
    lli                 r22, 0x1000             # r22 = distance between arrays
    addu                r12, ch0, r0            # r12 = rows of input
    addu                r13, ch0, r0            # r13 = columns of input
    mullw               r14, r13, r12           # r14 = length of input

# Initialisation
    addu                r10, r0,  r30           # r10 = current load location
    addu                r11, r10, r20           # r11 = first input array
    ldbu                0(r11)          -> 1    # load red
    addu                r11, r11, r22           # r11 = second input array
    ldbu                0(r11)          -> 1    # load green
    addu                r11, r11, r22           # r11 = third input array
    ldbu                0(r11)          -> 1    # load blue
    addu                r10, r10, r31           # move to next pixel
    addu.eop            r8,  r0,  r30           # r8 = current store location

# Start of loop
# Load subpixel values
load:
    addu                r17, ch0, r0            # r17 = red
    addu                r18, ch0, r0            # r18 = green
    addu                r19, ch0, r0            # r19 = blue
    addu                r11, r10, r20           # r11 = first input array
    ldbu                0(r11)          -> 1    # load red
    addu                r11, r11, r22           # r11 = second input array
    ldbu                0(r11)          -> 1    # load green
    addu                r11, r11, r22           # r11 = third input array
    ldbu                0(r11)          -> 1    # load blue
    fetchr              computey
    addu.eop            r10, r10, r31           # move to next pixel

# Compute Y
computey:
    fetchr              compute
    addui               r25, r0,  66            # store coefficient
    addui               r26, r0,  129           # store coefficient
    addui               r27, r0,  25            # store coefficient
    lli.eop             r29, computeu           # store next IPK location

# Store Y, compute U
computeu:
    addui               r25, r25, 16            # r25 = Y
    addu                r9,  r8,  r21           # r9 = first output array
    stb                 r25, 0(r9)      -> 1    # store Y
    fetchr              compute
    addui               r25, r0,  -38           # store coefficient
    addui               r26, r0,  -74           # store coefficient
    addui               r27, r0,  112           # store coefficient
    lli.eop             r29, computev           # store next IPK location

# Store U, compute V
computev:
    addui               r25, r25, 128           # r25 = U
    addu                r9,  r9,  r22           # r9 = second output array
    stb                 r25, 0(r9)      -> 1    # store U
    fetchr              compute
    addui               r25, r0,  112           # store coefficient
    addui               r26, r0,  -94           # store coefficient
    addui               r27, r0,  -18           # store coefficient
    lli.eop             r29, endofloop          # store next IPK location

# Store V, loop if necessary
endofloop:
    addui               r25, r25, 128           # r25 = V
    addu                r9,  r9,  r22           # r9 = third output array
    stb                 r25, 0(r9) -> 1         # store V
    addu                r8,  r8,  r31           # move to next pixel
    setlt.p             r0,  r8,  r14           # see if we have finished
    lli                 r25, load
    lli                 r26, exit
    psel.fetch.eop      r25, r26

# Only one core should call sys_exit, and it should make sure that all other
# cores have finished first.
exit:
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

# Subroutine to compute (xR + yG + zB + 128) >> 8
# Excludes final addition because I currently don't have enough registers for it.
# Assumes x is in r25, y is in r26, z is in r27 and next IPK address is in r29.
# Leaves result in r25.
compute:
    fetch               r29
    mullw               r25, r25, r17           # xR
    mullw               r26, r26, r18           # yG
    mullw               r27, r27, r19           # zB
    addu                r25, r25, r26           # xR + yG
    addu                r25, r25, r27           # xR + yG + zB
    addui               r25, r25, 128           # xR + yG + zB + 128
    srli.eop            r25, r25, 8             # (xR + yG + zB + 128) >> 8
