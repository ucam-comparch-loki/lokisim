# Matrix transpose.
#
# Reads the input column-by-column, so the output is written sequentially. This
# will allow us to make use of a streaming write operation when implemented.

# Load parameters
_start:
    fetchr              loop
    ldw                 4(r0)           -> 1
    ldw                 8(r0)           -> 1
    or                  r17, r0,  r0
    or                  r18, r0,  r0
    lui                 r17, 0x0001             # r17 = start of input  (0x10000)
    lui                 r18, 0x0002             # r18 = start of output (0x20000)
    ori                 r22, ch0, 0             # r22 = rows of input (columns of output)
    ori                 r23, ch0, 0             # r23 = columns of input (rows of output)
    slli.eop            r25, r23, 2             # r25 = offset to move down one row of input

# Start of outer loop
loop:
    ori                 r11, r0,  0             # r11 = current input column/output row
    ori                 r12, r0,  0             # r12 = position in current row/column
    slli                r13, r11, 2             # r13 = address to load from
    addu                r13, r13, r17
    ldw                 0(r13)          -> 1    # load matrix element
    addui               r12, r12, 1             # increment current element
    addu                r13, r13, r25           # increment load address
    mullw               r14, r11, r22
    slli                r14, r14, 2             # r14 = address to store to
    addu                r14, r14, r18

# Start of inner loop (go through one column of input)
    ldw                 0(r13)          -> 1    # load matrix element
    addui               r12, r12, 1             # increment current element
    stw                 ch0, 0(r14)     -> 1    # store received value
    addu                r13, r13, r25           # increment load address
    seteq.p             r0,  r12, r22           # see if we have finished this column
    addui               r14, r14, 4             # increment store address
    if!p?ibjmp          -24                     # continue along row if not finished
# End of inner loop

    stw                 ch0, 0(r14)     -> 1    # store received value
    addui               r11, r11, 1             # increment current input column
    setgte.p            r0,  r11, r23           # see if we have finished the whole matrix
    if!p?ibjmp          -76                     # if not, do another column
# End of outer loop

    syscall.eop         1
