# Load parameter(s)
simdstart:
    fetch               r0,  loop
    ldw                 4(r0)            -> 1   # load length of vectors
    slli                r11, r30, 2             # r11 = current position
    slli                r12, r31, 2             # r12 = stride length
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addui.eop           r10, r10, -4            # r10 = offset of final element

# Main loop
loop:
    ldw                 0x10000(r11)     -> 1   # load from the two vectors
    ldw                 0x11000(r11)     -> 1
    or                  r13, ch0, r0
    addu                r13, r13, ch0           # r13 = sum of vector elements
    stw                 r13, 0x20000(r11) -> 1  # store result
    addu                r11, r11, r12           # move to next element
    setlt.p             r0,  r10, r11           # see if we have work left to do
    if!p?ibjmp          -56
    seteq.p             r0,  r30, r0            # set p if we are core 0
    ifp?syscall         1                       # only core 0 exits (is this safe?)
    or.eop              r0,  r0,  r0
