# Set up connections to memories
_start:
    fetch               r0,  params
    ori                 r5,  r0,  (13,0)
    ori                 r6,  r0,  (14,0)
    ori                 r7,  r0,  (15,0)
    setchmap            1,   r5                 # vector 1 = map 1
    setchmap            2,   r6                 # vector 2 = map 2
    setchmap            3,   r7                 # output = map 3
    ori                 r0,  r0,  (0,2) -> 1
    ori                 r0,  r0,  (0,3) -> 2
    ori.eop             r0,  r0,  (0,0) -> 3

# Load parameter(s)
params:
    fetch               r0,  loop
    ldw                 4(r0)           -> 1
    ori                 r11, r0,  0             # r11 = current position
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addui.eop           r10, r10, -4            # r10 = offset of final element

# Main loop
loop:
    ldw                 8(r11)          -> 1
    ldw                 8(r11)          -> 2
    setlt.p             r0,  r11, r10           # see if we have work left to do
    addui               r11, r11, 4             # move to next element
    addu                r12, ch0, ch1           # r12 = sum of vector elements
    stw                 r12, -4(r11)    -> 3    # store result
    ifp?ibjmp           -48
    syscall.eop         1                       # exit
