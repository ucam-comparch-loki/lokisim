# Load parameter(s)
_start:
    fetch               r0,  loop
    ldw                 4(r0)           -> 1
    ori                 r11, r0,  0             # r11 = current load position
    ldw                 0x10000(r11)     -> 1   # load from the two vectors early
    ldw                 0x11000(r11)     -> 1
    addui               r11, r11, 4             # move to next element
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addui               r10, r10, -4            # r10 = offset of final element
    ori.eop             r14, r0,  0             # r14 = current store position

or.eop r0, r0, r0
or.eop r0, r0, r0

# Main loop
loop:
    or                  r12, ch0, r0
    addu                r12, r12, ch0           # r12 = sum of vector elements
    ldw                 0x10000(r11)    -> 1    # load from the two vectors
    ldw                 0x11000(r11)    -> 1
    addui               r11, r11, 4             # move to next element
    addui               r14, r14, 4
    setlt.p             r0,  r14, r10           # see if we have work left to do
    stw                 r12, 0x1FFFC(r14) -> 1  # store result (to 0x20000 - 4)
    ifp?ibjmp           -64

    or                  r12, ch0, r0
    addu                r12, r12, ch0           # r12 = sum of vector elements
    stw                 r12, 0x20000(r14) -> 1  # store result (to 0x20000)
    syscall.eop         1                       # exit
