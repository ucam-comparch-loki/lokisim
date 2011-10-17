# Load parameter(s)
_start:
    fetchr              loop
    ldw                 4(r0)           -> 1
    or                  r11, r0,  r0
    lui                 r11, 0x0001             # r11 = current load position (0x10000)
    ldw                 0(r11)          -> 1    # load from the two vectors early
    ldw                 0x1000(r11)     -> 1
    addui               r11, r11, 4             # move to next element
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addu                r10, r10, r11
    addui               r10, r10, -4            # r10 = offset of final element
    or                  r14, r0,  r0
    lui.eop             r14, 0x0002             # r14 = current store position (0x20000)

or.eop r0, r0, r0
or.eop r0, r0, r0

# Main loop
loop:
    or                  r12, ch0, r0
    addu                r12, r12, ch0           # r12 = sum of vector elements
    ldw                 0(r11)          -> 1    # load from the two vectors
    ldw                 0x1000(r11)     -> 1
    setlt.p             r0,  r11, r10           # see if we have work left to do
    addui               r11, r11, 4             # move to next element
    addui               r14, r14, 4
    stw                 r12, -4(r14)    -> 1    # store result (to 0x20000 - 4)
    ifp?ibjmp           -32

    or                  r12, ch0, r0
    addu                r12, r12, ch0           # r12 = sum of vector elements
    stw                 r12, 0(r14)     -> 1    # store result (to 0x20000)
    syscall.eop         1                       # exit
