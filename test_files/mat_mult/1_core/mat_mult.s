# Load parameters
_start:
    ldw                 4(r0)            -> 1
    ldw                 8(r0)            -> 1
    ldw                 12(r0)           -> 1
    fetchr              loop
    lli                 r8,  %lo(0x10000)
    lli                 r9,  %lo(0x20000)
    lli                 r10, %lo(0x30000)
    lui                 r8,  %hi(0x10000)       # r8 = start of input matrix 1
    lui                 r9,  %hi(0x20000)       # r9 = start of input matrix 2
    lui                 r10, %hi(0x30000)       # r10 = start of output matrix
    addu                r22, ch0, r0            # r22 = rows of matrix 1
    addu                r23, ch0, r0            # r23 = columns of matrix 1 (rows of mat 2)
    addu.eop            r24, ch0, r0            # r24 = columns of matrix 2

# Start of main loop
loop:
    addu                r11, r0,  r0            # r11 = current output row
    addu                r12, r0,  r0            # r12 = current output column
    addu                r13, r0,  r0            # r13 = result to store
    addu                r14, r0,  r0            # r14 = position in current row/col

# Compute where to load from in matrix 1
    mullw               r25, r23, r11           # start of row = row * num columns
    addu                r25, r25, r14           # element = start of row + column
    slli                r25, r25, 2             # get address
    addu                r25, r25, r8
    ldw                 0(r25)          -> 1    # load element (skipping over parameters)

# Compute where to load from in matrix 2
    mullw               r26, r24, r14           # start of row = num columns * row
    addu                r26, r26, r12           # element = start of row + column
    slli                r26, r26, 2             # get address
    addu                r26, r26, r9
    ldw                 0(r26)          -> 1    # load element

# Deal with inputs (and loop if necessary)
    addui               r14, r14, 1             # increment iteration variable
    seteq.p             r0,  r14, r23           # see if we will have finished this result
    addu                r27, ch0, r0            # receive first matrix element
    mullw               r27, r27, ch0           # multiply matrix elements
    addu                r13, r13, r27           # add result to total so far
    if!p?ibjmp          -60                     # if not finished, loop

# Compute where to store the result
    mullw               r27, r11, r24           # start of row = row * num columns
    addu                r27, r27, r12           # element = start of row + column
    slli                r27, r27, 2             # get address
    addu                r27, r27, r10
    stw                 r13, 0(r27)     -> 1    # store result

    addui               r12, r12, 1             # move to next output column
    seteq.p             r0,  r12, r24           # see if we have finished the row
    if!p?ibjmp          -100                    # if not, loop

    addui               r11, r11, 1             # move to next output row
    seteq.p             r0,  r11, r22           # see if we have finished the matrix
    if!p?ibjmp          -116                    # if not, loop

    syscall.eop         1                       # if so, end
