# Matrix transpose.
#
# Reads the input column-by-column, so the output is written sequentially. This
# will allow us to make use of a streaming write operation when implemented.

# Load parameters
simdstart:
    fetch               r0,  loop
    ldw                 4(r0)           -> 1
    ldw                 8(r0)           -> 1
    ori                 r2,  ch0, 0             # r2 = rows of input (columns of output)
    ori                 r3,  ch0, 0             # r3 = columns of input (rows of output)
    slli.eop            r5,  r3,  2             # r5 = offset to move down one row of input

# Start of outer loop
loop:
    ori                 r11, r30, 0             # r11 = current input column/output row
    ori                 r12, r0,  0             # r12 = position in current row/column
    slli                r13, r11, 2             # r13 = address to load from
    mullw               r14, r11, r2
    slli                r14, r14, 2             # r14 = address to store this row

# Start of inner loop (go through one column of input)
    ldw                 0x10000(r13)    -> 1    # load matrix element
    addui               r12, r12, 1             # increment current element
    addu                r13, r13, r5            # increment load address
    seteq.p             r0,  r12, r2            # see if we have finished this column
    stw                 ch0, 0x20000(r14) -> 1  # store received value
    addui               r14, r14, 4             # increment store address
    if!p?ibjmp          -48                     # continue along row if not finished
# End of inner loop

    addu                r11, r11, r31           # increment current input column
    setgte.p            r0,  r11, r3            # see if we have finished the whole matrix
    if!p?ibjmp          -104                    # if not, do another column
# End of outer loop

    seteq.p             r0,  r30, r0            # set p if we are core 0
    ifp?syscall         1
    or.eop              r0,  r0,  r0
