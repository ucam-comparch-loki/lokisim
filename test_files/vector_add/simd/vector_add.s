# Set up connections to memories
simdstart:
    fetch               r0,  params
    addui               r5,  r30, (9,0)         # each member will access a different port
    addui               r6,  r30, (10,0)
    addui               r7,  r30, (11,0)
    setchmap            1,   r5                 # vector 1 = map 1
    setchmap            2,   r6                 # vector 2 = map 2
    setchmap            3,   r7                 # output = map 3
    addui               r0,  r8,  (0,2)  -> 1   # compute which ports to return data to
    addui               r0,  r8,  (0,3)  -> 2
    addui.eop           r0,  r8,  (0,0)  -> 3

# Load parameter(s)
params:
    fetch               r0,  loop
    ldw                 4(r0)            -> 1   # load length of vectors
    slli                r11, r30, 2             # r11 = current position
    slli                r12, r31, 2             # r12 = stride length
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addui.eop           r10, r10, -4            # r10 = offset of final element

# Main loop
loop:
    ldw                 8(r11)           -> 1   # 8 offset because we skip over 2 arguments
    ldw                 8(r11)           -> 2
    addu                r13, ch0, ch1           # r13 = sum of vector elements
    stw                 r13, 0(r11)      -> 3   # store result
    addu                r11, r11, r12           # move to next element
    setlt.p             r0,  r10, r11           # see if we have work left to do
    if!p?ibjmp          -48
    syscall.eop         1
