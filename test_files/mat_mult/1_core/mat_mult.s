# Load parameters
_start:
    ldw                 4(r0)            -> 1
    ldw                 8(r0)            -> 1
    ldw                 12(r0)           -> 1
    fetch               r0,  loop
    ori                 r2,  ch0, 0             # r2 = rows of matrix 1
    ori                 r3,  ch0, 0             # r3 = columns of matrix 1 (rows of mat 2)
    ori.eop             r4,  ch0, 0             # r4 = columns of matrix 2

# Start of main loop
loop:
    ori                 r11, r0,  0             # r11 = current output row
    ori                 r12, r0,  0             # r12 = current output column
    ori                 r13, r0,  0             # r13 = result to store
    ori                 r14, r0,  0             # r14 = position in current row/col

# Compute where to load from in matrix 1
    mullw               r5,  r3,  r11           # start of row = row * num columns
    addu                r5,  r5,  r14           # element = start of row + column
    slli                r5,  r5,  2             # get address
    ldw                 0x10000(r5)      -> 1   # load element (skipping over parameters)

# Compute where to load from in matrix 2
    mullw               r6,  r4,  r14           # start of row = num columns * row
    addu                r6,  r6,  r12           # element = start of row + column
    slli                r6,  r6,  2             # get address
    ldw                 0x20000(r6)      -> 1   # load element

# Deal with inputs (and loop if necessary)
    addui               r14, r14, 1             # increment iteration variable
    seteq.p             r0,  r14, r3            # see if we will have finished this result
    or                  r7,  ch0, r0            # receive first matrix element
    mullw               r7,  r7,  ch0           # multiply matrix elements
    addu                r13, r13, r7            # add result to total so far
    if!p?ibjmp          -104                     # if not finished, loop

# Compute where to store the result
    mullw               r7,  r11, r4            # start of row = row * num columns
    addu                r7,  r7,  r12           # element = start of row + column
    slli                r7,  r7,  2             # get address
    stw                 r13, 0x30000(r7)  -> 1  # store result

    addui               r12, r12, 1             # move to next output column
    seteq.p             r0,  r12, r4            # see if we have finished the row
    if!p?ibjmp          -176                    # if not, loop

    addui               r11, r11, 1             # move to next output row
    seteq.p             r0,  r11, r2            # see if we have finished the matrix
    if!p?ibjmp          -208                    # if not, loop

    syscall.eop         1                       # if so, end
