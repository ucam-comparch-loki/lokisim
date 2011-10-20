# Load parameter(s)
simdstart:
    fetchr              loop
    ldw                 4(r0)           -> 1    # load length of vectors
    lli                 r20, %lo(0x10000)
    lui                 r20, %hi(0x10000)       # r20 = position of input
    lli                 r21, %lo(0x20000)
    lui                 r21, %hi(0x20000)       # r21 = position of output
    slli                r11, r30, 2             # r11 = start offset
    addu                r14, r11, r20           # r14 = load position
    addu                r15, r11, r21           # r15 = store position
    ldw                 0(r14)          -> 1    # load from the two vectors early
    ldw                 0x1000(r14)     -> 1
    slli                r12, r31, 2             # r12 = stride length
    addu                r14, r14, r12           # move to next element
    slli                r10, ch0, 2             # r10 = byte-length of vectors
    addu                r10, r10, r20
    addui.eop           r10, r10, -4            # r10 = offset of final element

# Main loop
loop:
    addu                r13, ch0, r0
    addu                r13, r13, ch0           # r13 = sum of vector elements
    ldw                 0(r14)          -> 1    # load from the two vectors
    ldw                 0x1000(r14)     -> 1
    addu                r14, r14, r12           # move to next element
    stw                 r13, 0(r15)     -> 1    # store result
    addu                r15, r15, r12           # move to next element
    setlt.p             r0,  r10, r14           # see if we have work left to do
    if!p?ibjmp          -32

    addu                r13, ch0, r0
    addu                r13, r13, ch0           # r13 = sum of vector elements
    stw                 r13, 0(r15)     -> 1    # final late store
    seteq.p             r0,  r30, r0            # set p if we are core 0
    ifp?syscall         1                       # only core 0 exits (is this safe?)
    addu.eop            r0,  r0,  r0
