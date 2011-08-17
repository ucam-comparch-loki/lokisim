# So that absolute positions can be used in the SIMD setup code, have a constant
# size packet here which just fetches the main helper program.
hackystart:
    fetch.eop               r0,  helpercode

# Receive parameters
simdstart:
#    fetch               r0,  loadrow
    or                  r2,  ch5, r0            # r2 = rows of matrix 1
    or                  r3,  ch5, r0            # r3 = columns of matrix 1 (rows of mat 2)
    or                  r4,  ch5, r0            # r4 = columns of matrix 2
    addui               r11, r30, -1            # r11 = current output row
    fetch.eop           r0,  loadrow

# Load row of matrix 1 into IRF
loadrow:
#    fetch               r0,  loop
    mullw               r5,  r3,  r11
    slli                r5,  r5,  2             # computed location of start of row
    ldw                 r5,  0x10000     > 1    # load a value
    addui               r5,  r5,  4             # move to next value

    ldw                 r5,  0x10000     > 1    # load a value
    addui               r5,  r5,  4             # move to next value
    iwtr                ch5, ch0                # store the value in the IRF
    setne.p             r0,  ch5,  r0
    ifp?ibjmp           -32                     # if so, load another

    iwtr                ch5, ch0
    fetch.eop           r0,  loop

# Start of main loop
loop:
    ori                 r12, r0,  0             # r12 = current output column
    ori                 r13, r0,  0             # r13 = result to store

# Retrieve matrix 1 value from IRF
    irdr                r7,  ch5

# Deal with inputs (and loop if necessary)
    setne.p             r0,  ch5, r0            # receive predicate
    mullw               r7,  r7,  ch5           # multiply matrix elements
    addu                r13, r13, r7            # add result to total so far
    if!p?ibjmp          -32                     # if not finished, loop

    irdr                r7,  ch5
    mullw               r7,  r7,  ch5           # multiply matrix elements
    addu                r13, r13, r7            # add result to total so far

# Compute where to store the result
    mullw               r7,  r11, r4            # start of row = row * num columns
    addu                r7,  r7,  r12           # element = start of row + column
    slli                r7,  r7,  2             # get address
    stw                 r13, r7,  0x30000 > 1   # store result

    setne.p             r0,  ch5, r0            # receive predicate
    addui               r12, r12, 1             # move to next output column
    if!p?ibjmp          -120                     # if not, loop

    setne.p             r0,  ch5, r0            # receive predicate
    addu                r11, r11, r31           # move to next output row
    ifp?fetch           r0,  end
    if!p?fetch          r0,  loadrow
    or.eop              r0,  r0,  r0

end:
    seteqi.p            r0,  r30, 1             # set p if we are core 1
    ifp?syscall         1                       # if so, end
    or.eop              r0,  r0,  r0



# Load parameters
helpercode:
    ldw                 r0,  4           > 1
    ldw                 r0,  8           > 1
    ldw                 r0,  12          > 1
    or                  r2,  ch0, r0     > 3    # r2 = rows of matrix 1
    or                  r3,  ch0, r0     > 3    # r3 = columns of matrix 1 (rows of mat 2)
    or                  r4,  ch0, r0     > 3    # r4 = columns of matrix 2
    or                  r11, r0,  r0            # r11 = current output row
    fetch.eop           r0,  helploadrow

helploadrow:
    ori                 r10, r0,  32            # r10 = pointer into IRF
    addu                r12, r10, r3            # r12 = register to stop putting values in
    addui               r12, r12, -1

    or                  r0,  r10, r0     > 3    # send IRF index to group
    addui               r10, r10, 1             # move to next register
    setlt.p             r0,  r10, r12    > 3    # see if we have values left to load
    ifp?ibjmp           -24                     # if so, load another

    or                  r0,  r10, r0     > 3
    fetch.eop           r0,  helploop

helploop:
    or                  r12, r0,  r0            # r12 = current output column
    or                  r14, r0,  r0            # r14 = position in current row/col
    or                  r15, r0,  r0            # r15 = current input row

# Load early from matrix 2
    mullw               r6,  r4,  r15           # start of row = num columns * row
    addu                r6,  r6,  r12           # element = start of row + column
    slli                r6,  r6,  2             # get address
    ldw                 r6,  0x20000     > 1    # load element
    addui               r15, r15, 1

# Compute where to load from in matrix 2
    mullw               r6,  r4,  r15           # start of row = num columns * row
    addu                r6,  r6,  r12           # element = start of row + column
    slli                r6,  r6,  2             # get address
    ldw                 r6,  0x20000     > 1    # load element
    addui               r15, r15, 1

    addui               r0,  r14, 32     > 3    # tell group which register to load from
    addui               r14, r14, 1             # increment iteration variable

    seteq.p             r0,  r15, r3     > 3    # see if we will have finished this result
    or                  r0,  ch0, r0     > 3    # send element of matrix 2
    if!p?ibjmp          -72

# Send final register/matrix element late
    addui               r0,  r14, 32     > 3    # tell group which register to load from
    or                  r0,  ch0, r0     > 3    # send element of matrix 2

    addui               r12, r12, 1             # move to next output column
    seteq.p             r0,  r12, r4     > 3    # see if we have finished the row
    if!p?ibjmp          -168                    # if not, loop

    addu                r11, r11, r31           # move to next output row
    setgte.p            r0,  r11, r2     > 3    # see if we have finished the matrix
    ifp?fetch           r0,  end
    if!p?fetch          r0,  helploadrow
    or.eop              r0,  r0,  r0
