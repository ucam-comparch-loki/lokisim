# Matrix transpose.
#
# Reads the input column-by-column, so the output is written sequentially. This
# is currently the best way of doing this because we have a streaming write
# instruction (stwaddr), but no streaming read instruction.

# Set up connections to memories
_start:
    fetch               r1,  56
    ori                 r5,  r0,  (9,0)
    ori                 r6,  r0,  (10,0)
    setchmap            1,   r5                 # input = map 1
    setchmap            2,   r6                 # output = map 2
    ori                 r0,  r0,  (0,2)  > 1
    ori.eop             r0,  r0,  (0,0)  > 2

# Load parameters
params:
    fetch               r1,  48
    ldw                 r0,  4           > 1
    ldw                 r0,  8           > 1
    ori                 r2,  ch0, 0             # r2 = rows of input (columns of output)
    ori                 r3,  ch0, 0             # r3 = columns of input (rows of output)
    slli.eop            r5,  r3,  2             # r5 = offset to move down one row of input

# Start of outer loop
loop:
    ori                 r11, r0,  0             # r11 = current input column/output row
    ori                 r12, r0,  0             # r12 = position in current row/column
    slli                r13, r11, 2             # r13 = address to load from
    mullw               r14, r11, r2
    slli                r14, r14, 2             # r14 = address to store to

# Start of inner loop (go through one column of input)
    ldw                 r13, 12          > 1    # load matrix element (skipping over params)
    addui               r12, r12, 1             # increment current element
    addu                r13, r13, r5            # increment load address
    seteq.p             r0,  r12, r2            # see if we have finished this column
    stw                 ch0, r14, 0      > 2    # store received value
    addui               r14, r14, 4             # increment store address
    !p?ibjmp            -48                     # continue along row if not finished
# End of inner loop

    addui               r11, r11, 1             # increment current input column
    setgte.p            r0,  r11, r3            # see if we have finished the whole matrix
    !p?ibjmp            -104                    # if not, do another column
# End of outer loop

    syscall.eop         1
