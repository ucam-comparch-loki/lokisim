# Matrix transpose.
#
# Reads the input column-by-column, so the output is written sequentially. This
# is currently the best way of doing this because we have a streaming write
# instruction (stwaddr), but no streaming read instruction.

# So that absolute positions can be used in the SIMD setup code, have a constant
# size packet here which just fetches the main helper program.
hackystart:
    fetch.eop               r0,  helpercode

# Load parameters
simdstart:
#    fetch               r0,  loop
    ori                 r2,  ch5, 0             # r2 = rows of input (columns of output)
    ori                 r3,  ch5, 0             # r3 = columns of input (rows of output)
    slli                r5,  r3,  2             # r5 = offset to move down one row of input
    fetch.eop           r0,  loop

# Start of outer loop
loop:
    addui               r11, r30, -1            # r11 = current input column/output row
    slli                r13, r11, 2             # r13 = address to load from
    ldw                 r13, 0x10000     > 1    # load matrix element
    addu                r13, r13, r5            # increment load address
    or                  r0,  ch5, r0            # ignore predicate from helper core
    mullw               r14, r11, r2
    slli                r14, r14, 2             # r14 = address to store this row

# Start of inner loop (go through one column of input)
    ldw                 r13, 0x10000     > 1    # load matrix element
    addu                r13, r13, r5            # increment load address
    setne.p             r0,  ch5, r0            # receive predicate from helper core
    stw                 ch0, r14, 0x20000 > 1   # store received value
    addui               r14, r14, 4             # increment store address
    if!p?ibjmp          -40                     # continue along row if not finished
# End of inner loop

    stw                 ch0, r14, 0x20000 > 1   # store received value
    setne.p             r0,  ch5, r0            # receive predicate from helper core
    addu                r11, r11, r31           # increment current input column
    if!p?ibjmp          -120                    # do another column if not finished
# End of outer loop

    fetch.eop           r0,  continuecheck
#    fetch.eop           r0,  end

# Determine whether each core should carry on in the group, or end now.
continuecheck:
    or                  r29, ch5, r0            # receive new member mask
    srl.p               r0,  r29, r30           # set p if still a member
    if!p?fetch          r0,  end                # if not a member, end
    ifp?fetch           r0,  lastiteration      # if a member, do one final iteration
    or.eop              r0,  r0,  r0

# A final bit of near-identical code to make sure the last iteration is neatly
# finished off.
lastiteration:
    ori                 r12, r0,  0             # r12 = position in current row/column
    slli                r13, r11, 2             # r13 = address to load from
    ldw                 r13, 0x10000     > 1    # load matrix element
    addui               r12, r12, 1             # increment current element
    addu                r13, r13, r5            # increment load address
    mullw               r14, r11, r2
    slli                r14, r14, 2             # r14 = address to store this row

# Go through one column of input
    ldw                 r13, 0x10000     > 1    # load matrix element
    addui               r12, r12, 1             # increment current element
    addu                r13, r13, r5            # increment load address
    seteq.p             r0,  r12, r2            # see if we have finished this column
    stw                 ch0, r14, 0x20000 > 1   # store received value
    addui               r14, r14, 4             # increment store address
    if!p?ibjmp          -48                     # continue along row if not finished
    stw                 ch0, r14, 0x20000 > 1   # store received value
    fetch.eop           r0,  end

end:
    seteqi.p            r0,  r30, 1             # set p if we are core 1
    ifp?syscall         1
    or.eop              r0,  r0,  r0



# Code to be run by the single helper core.
helpercode:
    ldw                 r0,  4           > 1
    ldw                 r0,  8           > 1
    ori                 r2,  ch0, 0      > 3    # r2 = rows of input (columns of output)
    ori                 r3,  ch0, 0      > 3    # r3 = columns of input (rows of output)
    addui               r11, r31, -1            # r11 = maximum current row/column
#    or                  r11, r0,  0             # r11 = current row/column

    or                  r12, r0,  r0            # r12 = position in current row/column
    addui               r12, r12, 1
    seteq.p             r0,  r12, r2     > 3    # see if we have finished this column
    if!p?ibjmp          -16                     # continue along row if not finished

    addu                r11, r11, r31           # update row/column
    setgte.p            r0,  r11, r3     > 3    # see if we have finished the whole matrix
    if!p?ibjmp          -48                     # do another column if not finished

    fetch.eop           r0,  newmask
#    or.eop              r0,  r0,  r0            # option if not having special final iteration

# Some cores may still have one more iteration to complete. Send out a bitmask
# to tell them which ones.
newmask:
    subu                r10, r11, r31           # r10 = num columns completed
    subu                r13, r3,  r10           # r13 = num columns uncompleted (+1)
    ori                 r14, r0,  1
    sll                 r14, r14, r13
    addui               r14, r14, -1            # r14 = mask to apply to existing group
    and.eop             r29, r29, r14    > 3    # apply mask and send to everyone
