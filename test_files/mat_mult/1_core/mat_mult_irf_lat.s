# Load parameters
_start:
    ldw                 4(r0)          -> 1
    ldw                 8(r0)          -> 1
    ldw                 12(r0)         -> 1
    fetch               r0,  loadrow
    ori                 r2,  ch0, 0             # r2 = rows of matrix 1
    ori                 r3,  ch0, 0             # r3 = columns of matrix 1 (rows of mat 2)
    ori                 r4,  ch0, 0             # r4 = columns of matrix 2
    ori                 r11, r0,  0             # r11 = current output row
    ori                 r25, r0,  loadrow
    ori.eop             r26, r0,  end           # some branch addresses for later

# Load row of matrix 1 into IRF
loadrow:
    fetch               r0,  loop
    ori                 r10, r0,  32            # r10 = pointer into IRF
    addu                r12, r10, r3            # r12 = register to stop putting values in
    mullw               r5,  r3,  r11
    slli                r5,  r5,  2             # computed location of start of row
    ldw                 0x10000(r5)     -> 1    # load a value
    addui               r5,  r5,  4             # move to next value (move to before jump?)

    ldw                 0x10000(r5)     -> 1    # load a value
    addui               r5,  r5,  4             # move to next value (move to before jump?)
    iwtr                r10, ch0                # store the value in the IRF
    addui               r10, r10, 1             # move to next register
    setlt.p             r0,  r10, r12           # see if we have values left to load
    ifp?ibjmp           -40                     # if so, load another

    iwtr.eop            r10, ch0                # store the value in the IRF

# Start of main loop
loop:
    ori                 r12, r0,  0             # r12 = current output column
    ori                 r13, r0,  0             # r13 = result to store
    ori                 r14, r0,  0             # r14 = position in current row/col
    ori                 r15, r0,  0             # r15 = current input row

# Load from matrix 2 early
    mullw               r6,  r4,  r15           # start of row = num columns * row
    addu                r6,  r6,  r12           # element = start of row + column
    slli                r6,  r6,  2             # get address
    ldw                 0x20000(r6)     -> 1    # load element
    addui               r15, r15, 1

# Compute where to load from in matrix 2
    mullw               r6,  r4,  r15           # start of row = num columns * row
    addu                r6,  r6,  r12           # element = start of row + column
    slli                r6,  r6,  2             # get address
    ldw                 0x20000(r6)     -> 1    # load element
    addui               r15, r15, 1

# Retrieve matrix 1 value from IRF
    addui               r7,  r14, 32
    addui               r14, r14, 1             # increment iteration variable
    irdr                r7,  r7

# Deal with inputs (and loop if necessary)
    seteq.p             r0,  r15, r3            # see if we will have finished this result
    mullw               r7,  r7,  ch0           # multiply matrix elements
    addu                r13, r13, r7            # add result to total so far
    if!p?ibjmp          -88                     # if not finished, loop

# Receive one last value from network
    addui               r7,  r14, 32
    irdr                r7,  r7
    mullw               r7,  r7,  ch0           # multiply matrix elements
    addu                r13, r13, r7            # add result to total so far

# Compute where to store the result
    mullw               r7,  r11, r4            # start of row = row * num columns
    addu                r7,  r7,  r12           # element = start of row + column
    slli                r7,  r7,  2             # get address
    stw                 r13, 0x30000(r7) -> 1   # store result

    addui               r12, r12, 1             # move to next output column
    seteq.p             r0,  r12, r4            # see if we have finished the row
    if!p?ibjmp          -240                    # if not, loop

    addui               r11, r11, 1             # move to next output row
    seteq.p             r0,  r11, r2            # see if we have finished the matrix

    psel.fetch.eop      r26, r25                # load another row, or end

end:
    syscall.eop         1                       # if so, end
