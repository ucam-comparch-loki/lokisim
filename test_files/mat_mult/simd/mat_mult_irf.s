# Set up connections to memories
simdstart:
    fetch               r0,  params
    addui               r5,  r30, (9,0)
    addui               r6,  r30, (10,0)
    addui               r7,  r30, (11,0)
    setchmap            1,   r5                 # matrix 1 = map 1
    setchmap            2,   r6                 # matrix 2 = map 2
    setchmap            3,   r7                 # output = map 3
    addui               r0,  r8,  (0,2)  > 1
    addui               r0,  r8,  (0,3)  > 2
    addui.eop           r0,  r8,  (0,0)  > 3

# Load parameters
params:
    fetch               r0,  loadrow
    ldw                 r0,  4           > 1
    ldw                 r0,  8           > 1
    ldw                 r0,  12          > 1
    ori                 r2,  ch0, 0             # r2 = rows of matrix 1
    ori                 r3,  ch0, 0             # r3 = columns of matrix 1 (rows of mat 2)
    ori                 r4,  ch0, 0             # r4 = columns of matrix 2
    ori.eop             r11, r30,  0            # r11 = current output row

# Load row of matrix 1 into IRF
loadrow:
    fetch               r0,  loop
    ori                 r10, r0,  32            # r10 = pointer into IRF
    addu                r12, r10, r3            # r12 = register to stop putting values in
    mullw               r5,  r3,  r11
    slli                r5,  r5,  2             # computed location of start of row
    ldw                 r5,  16,         > 1    # load a value
    addui               r5,  r5,  4             # move to next value
    iwtr                r10, ch0                # store the value in the IRF
    addui               r10, r10, 1             # move to next register
    setlt.p             r0,  r10, r12           # see if we have values left to load
    ifp?ibjmp           -40                     # if so, load another
    or.eop              r0,  r0,  r0

# Start of main loop
loop:
    ori                 r12, r0,  0             # r12 = current output column
    ori                 r13, r0,  0             # r13 = result to store
    ori                 r14, r0,  0             # r14 = position in current row/col

# Compute where to load from in matrix 2
    mullw               r6,  r4,  r14           # start of row = num columns * row
    addu                r6,  r6,  r12           # element = start of row + column
    slli                r6,  r6,  2             # get address
    ldw                 r6,  0           > 2    # load element

# Retrieve matrix 1 value from IRF
    addui               r7,  r14, 32
    addui               r14, r14, 1             # increment iteration variable
    irdr                r7,  r7

# Deal with inputs (and loop if necessary)
    seteq.p             r0,  r14, r3            # see if we will have finished this result
    mullw               r7,  r7,  ch1           # multiply matrix elements
    addu                r13, r13, r7            # add result to total so far
    if!p?ibjmp          -80                     # if not finished, loop

# Compute where to store the result
    mullw               r7,  r11, r4            # start of row = row * num columns
    addu                r7,  r7,  r12           # element = start of row + column
    slli                r7,  r7,  2             # get address
    stw                 r13, r7,  0      > 3    # store result

    addui               r12, r12, 1             # move to next output column
    seteq.p             r0,  r12, r4            # see if we have finished the row
    if!p?ibjmp          -152                    # if not, loop

    addu                r11, r11, r31           # move to next output row
    setgte.p            r0,  r11, r2            # see if we have finished the matrix
    if!p?fetch          r0,  loadrow            # if not, loop

    syscall.eop         1                       # if so, end
