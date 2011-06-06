
# Load parameter(s)
_start:
#    fetch               r0,  loop
    ldw                 4(r0)           -> 1
    ori                 r11, r0,  0             # r11 = current position
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addui               r10, r10, -4            # r10 = offset of final element
    fetch.eop           r0,  loop

# Main loop
loop:
    ldw                 0x10000(r11)    -> 1
    ldw                 0x11000(r11)    -> 1
    setlt.p             r0,  r11, r10           # see if we have work left to do
    addui               r11, r11, 4             # move to next element
    or                  r12, ch0, r0
    addu                r12, r12, ch0           # r12 = sum of vector elements
    stw                 r12, 0x1FFFC(r11) -> 1  # store result (to 0x20000 - 4)
    ifp?ibjmp           -56
    syscall.eop         1                       # exit
