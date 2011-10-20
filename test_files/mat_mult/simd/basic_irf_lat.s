# Load parameters
simdstart:
    fetchr              loadrow
    ldw                 4(r0)          -> 1
    ldw                 8(r0)          -> 1
    ldw                 12(r0)         -> 1
    lli                 r8,  %lo(0x10000)
    lli                 r9,  %lo(0x20000)
    lli                 r10, %lo(0x30000)
    lui                 r8,  %hi(0x10000)       # r8 = start of input matrix 1
    lui                 r9,  %hi(0x20000)       # r9 = start of input matrix 2
    lui                 r10, %hi(0x30000)       # r10 = start of output matrix
    addu                r22, ch0, r0            # r22 = rows of matrix 1
    addu                r23, ch0, r0            # r23 = columns of matrix 1 (rows of mat 2)
    addu                r24, ch0, r0            # r24 = columns of matrix 2
    addu                r11, r30, 0             # r11 = current output row
    lli                 r20, loadrow
    lli.eop             r21, end                # some branch addresses for later

# Load row of matrix 1 into IRF
loadrow:
    fetchr              loop
    addui               r16, r0,  32            # r16 = pointer into IRF
    addu                r12, r16, r23           # r12 = register to stop putting values in
    mullw               r25, r23, r11
    slli                r25, r25, 2             # computed location of start of row
    addu                r25, r25, r8
    ldw                 0(r25)          -> 1    # load a value
    addui               r25, r25, 4             # move to next value

    iwtr                r16, ch0                # store the value in the IRF
    ldw                 0(r25)          -> 1    # load a value
    addui               r25, r25, 4             # move to next value
    addui               r16, r16, 1             # move to next register
    setlt.p             r0,  r16, r12           # see if we have values left to load
    ifp?ibjmp           -20                     # if so, load another

    iwtr.eop            r16, ch0                # store the value in the IRF

# Start of main loop
loop:
    addu                r12, r0,  r0            # r12 = current output column
    addu                r13, r0,  r0            # r13 = result to store
    addu                r14, r0,  r0            # r14 = position in current row/col
    addu                r15, r0,  r0            # r15 = current input row

# Load from matrix 2 early
    mullw               r26, r24, r15           # start of row = num columns * row
    addu                r26, r26, r12           # element = start of row + column
    slli                r26, r26, 2             # get address
    addu                r26, r26, r9
    ldw                 0(r26)          -> 1    # load element
    addui               r15, r15, 1

# Compute where to load from in matrix 2
    mullw               r26, r24, r15           # start of row = num columns * row
    addu                r26, r26, r12           # element = start of row + column
    slli                r26, r26, 2             # get address
    addu                r26, r26, r9
    ldw                 0(r26)          -> 1    # load element
    addui               r15, r15, 1

# Retrieve matrix 1 value from IRF
    addui               r27, r14, 32
    addui               r14, r14, 1             # increment iteration variable
    irdr                r27, r27

# Deal with inputs (and loop if necessary)
    seteq.p             r0,  r15, r23           # see if we will have finished this result
    mullw               r27, r27, ch0           # multiply matrix elements
    addu                r13, r13, r27           # add result to total so far
    if!p?ibjmp          -48                     # if not finished, loop

# Receive one last value from network
    addui               r27, r14, 32
    irdr                r27, r27
    mullw               r27, r27, ch0           # multiply matrix elements
    addu                r13, r13, r27           # add result to total so far

# Compute where to store the result
    mullw               r27, r11, r24           # start of row = row * num columns
    addu                r27, r27, r12           # element = start of row + column
    slli                r27, r27, 2             # get address
    addu                r27, r27, r10
    stw                 r13, 0(r27)     -> 1    # store result

    addui               r12, r12, 1             # move to next output column
    seteq.p             r0,  r12, r24           # see if we have finished the row
    if!p?ibjmp          -132                    # if not, loop

    addu                r11, r11, r31           # move to next output row
    setgte.p            r0,  r11, r22           # see if we have finished the matrix
    psel.fetch.eop      r21, r20

end:
    seteq.p             r0,  r30, r0            # set p if we are core 0
    ifp?syscall         1                       # if so, end
    addu.eop            r0,  r0,  r0
