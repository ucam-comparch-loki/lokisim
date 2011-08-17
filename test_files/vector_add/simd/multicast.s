# So that absolute positions can be used in the SIMD setup code, have a constant
# size packet here which just fetches the main helper program.
hackystart:
    fetch.eop               r0,  helpercode

# Load parameter(s)
simdstart:
#    fetch               r0,  loop
    addui               r11, r30, -1            # core 0 does no work => subtract 1
    slli                r11, r11, 2             # r11 = current position
    slli                r12, r31, 2             # r12 = stride length
    fetch.eop           r0,  loop

# Main loop
loop:
    ldw                 0x10000(r11)     -> 1   # load from the two vectors
    ldw                 0x11000(r11)     -> 1
    setne.p             r0,  ch5, r0            # see if we should do another iteration
    or                  r13, ch0, r0
    addu                r13, r13, ch0           # r13 = sum of vector elements
    stw                 r13, 0x20000(r11) -> 1  # store result
    addu                r11, r11, r12           # move to next element
    if!p?ibjmp          -56
    seteqi.p            r0,  r30, 1             # set p if we are core 1
    ifp?syscall         1                       # only core 1 exits
    or.eop              r0,  r0,  r0



helpercode:
#    fetch               r0,  helploop
    ldw                 4(r0)            -> 1   # load length of vectors
    or                  r11, r0,  r0            # r11 = current position
    addui               r10, ch0, -1            # r10 = index of final element
    fetch.eop           r0,  helploop

# Main loop
helploop:
    addu                r11, r11, r31           # move to next element
    setlt.p             r0,  r10, r11    -> 3   # see if we have work left to do
    if!p?ibjmp          -16
    or.eop              r0,  r0,  r0
