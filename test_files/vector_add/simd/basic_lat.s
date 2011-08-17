# Load parameter(s)
simdstart:
#    fetch               r0,  loop
    ldw                 4(r0)            -> 1   # load length of vectors
    slli                r11, r30, 2             # r11 = current position
    ldw                 0x10000(r11)     -> 1   # load from the two vectors early
    ldw                 0x11000(r11)     -> 1
    slli                r12, r31, 2             # r12 = stride length
    addu                r11, r11, r12           # move to next element
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addui               r10, r10, -4            # r10 = offset of final element
    slli                r14, r30, 2             # r14 = current store position
    fetch.eop           r0,  loop

or.eop r0 r0 r0

# Main loop
loop:
    or                  r13, ch0, r0
    addu                r13, r13, ch0           # r13 = sum of vector elements
    ldw                 0x10000(r11)     -> 1   # load from the two vectors
    ldw                 0x11000(r11)     -> 1
    addu                r11, r11, r12           # move to next element
    stw                 r13, 0x20000(r14) -> 1  # store result
    addu                r14, r14, r12           # move to next element
    setlt.p             r0,  r10, r14           # see if we have work left to do
    if!p?ibjmp          -64
    seteq.p             r0,  r30, r0            # set p if we are core 0
    ifp?syscall         1                       # only core 0 exits (is this safe?)
    or.eop              r0,  r0,  r0
