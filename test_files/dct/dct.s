_start:
#    fetch           r0,  tocore1
    ori             r11, r0,  (0,1,0)
    ori             r12, r0,  (0,2,0)
    ori             r13, r0,  (0,1,2)
    ori             r14, r0,  (0,2,2)
    setchmap        1,   r11                # map 1 = cluster 1 instructions
    setchmap        2,   r12                # map 2 = cluster 2 instructions
    setchmap        3,   r13                # map 3 = cluster 1 data
    setchmap        4,   r14                # map 4 = cluster 2 data
    or              r0,  r3,  r0  > 3       # send cache configuration to core 1
    or              r0,  r3,  r0  > 4       # send cache configuration to core 2
    fetch.eop       r0,  tocore1

tocore1:
#    fetch           r0,  tocore2
    rmtexecute                    > 1       # start sending instructions to cluster 1
    ifp?addui       r4,  ch0, (0,0,2,0,0)   # receive/modify cache configuration...
    ifp?addui       r5,  r4,  (0,0,1,0,0)   # ...for the channels we will connect to
    ifp?ori         r9,  r0,  (0,2,2)
    ifp?setchmap    0,   r4                 # instruction memory connection = map 0
    ifp?setchmap    1,   r5                 # data memory connection = map 1
    ifp?ori         r0,  r0,  0x01000000 > 0  # tell memory we're about to connect
    ifp?ori         r0,  r0,  (0,1,1) > 0   # connect to instruction channel
    ifp?ori         r0,  r0,  0x01000000 > 1  # tell memory we're about to connect
    ifp?ori         r0,  r0,  (0,1,3) > 1   # connect to data channel
    ifp?setchmap    3,   r9                 # next core in pipeline = map 3
    ifp?or          r0,  ch1, r0            # receive sync message from memory
    ifp?fetch       r0,  core1
    ori             r0,  r0,  0x10200 > 3   # Tell Cluster 1 where its first block is
    ori             r0,  r0,  0x20000 > 3   # Tell Cluster 1 where to store its output
    fetch.eop       r0,  tocore2

tocore2:
    rmtexecute                    > 2       # start sending instructions to cluster 2
    ifp?addui       r4,  ch0, (0,0,4,0,0)   # receive/modify cache configuration...
    ifp?addui       r5,  r4,  (0,0,1,0,0)   # ...for the channels we will connect to
    ifp?setchmap    0,   r4                 # instruction memory connection = map 0
    ifp?setchmap    1    r5                 # data memory connection = map 1
    ifp?ori         r0,  r0,  0x01000000 > 0  # tell memory we're about to connect
    ifp?ori         r0,  r0,  (0,2,1) > 0   # connect to instruction channel
    ifp?ori         r0,  r0,  0x01000000 > 1  # tell memory we're about to connect
    ifp?ori         r0,  r0,  (0,2,3) > 1   # connect to data channel
    ifp?or          r0,  ch1, r0            # receive sync message from memory
    ifp?fetch       r0,  core2
    or.eop          r0,  r0,  r0


# 1D DCT along rows

# Prologue: connect to data memories
core1:
    or          r30, ch0, r0            # wait for location of input block
    or          r31, ch0, r0            # wait for location of output block
    ori         r29, r0,  11585         # store a value we need

# Initialisation: receive parameters
    and         r2,  r2,  r0            # current row = 0
    and         r3,  r3,  r0            # current column = 0
    and         r4,  r4,  r0            # index of current weight = 0 (k)
    and         r5,  r5,  r0            # DCT output = 0 (f_val)
    and         r6,  r6,  r0            # values added to output so far = 0 (i)

# Inner loop
    slli        r7,  r2,  3             # current row * 8 = index of start of row
    addu        r8,  r7,  r6            # r7 now contains the position in the block of the pixel we want
    slli        r8,  r8,  2             # shift to get a byte address
    addu        r8,  r8,  r30           # r8 now holds the location of the pixel in memory
    ldw         r8,  0        > 1       # load the pixel (perhaps use ldb?)
    ldw         r4,  0x10000  > 1       # load the weight for this pixel
    or          r8,  ch1, r0            # store whichever value arrives first
    mullw       r8,  ch1, r8            # multiply by the value which arrives second
    addu        r5,  r5,  r8            # add to the output
    addui       r6,  r6,  1             # increment values added
    seteqi.p    r0,  r6,  8             # set predicate if we've added 8 values
    addui       r4,  r4,  4             # move to the next weight (filling predicate delay slot)
    if!p?ibjmp  -96                     # restart loop if we're not finished

    setnei.p    r0,  r3,  0             # set predicate if we are not in the first column
    addu        r8,  r7,  r3            # position in block to write to
    slli        r8,  r8,  2             # shift so we get a byte position
    addu        r8,  r8,  r31           # memory location to write to
    p?or        r9,  r5,  r0            # if p, want to write sum relatively unchanged
    if!p?srai   r9,  r5,  14            # else, need to do some fancy tricks with the sum
    if!p?mullw  r9,  r9,  r29
    srai        r9,  r9,  15            # the shift is common to both branches
    stw         r9,  r8,  0   > 1       # store the value

    addui       r3,  r3,  1             # increment current column
    seteqi.p    r0,  r3,  8             # set predicate if we've finished all 8 columns in this row
    if!p?ibjmp  -208                    # jump back to start of loop (f_val = 0)

    addui       r2,  r2,  1             # increment current row
    seteqi.p    r0,  r2,  8             # set predicate if we've finished all 8 rows
    or          r0,  r0,  r0           #  can't remove this nop for some reason
    if!p?ibjmp  -256                    # jump back to start of loop (column = 0)

    or.eop      r0,  r31, r0  > 3       # send a pointer to the output block to the next core


# 1D DCT along columns
# Note: instead of using this, the results of the first 1D DCT could be
# transposed, and fed back into the same 1D DCT.
# This might save a little code, but would probably use more cores, without
# speeding up execution.

# Prologue: connect to data memories
core2:
    ori         r29, r0,  11585         # store a value we need

# Initialisation: receive parameters
    or          r30, ch0, r0            # wait for location of input/output block

    and         r2,  r2,  r0            # current column = 0
    and         r3,  r3,  r0            # current row = 0
    and         r4,  r4,  r0            # index of current weight = 0 (k)
    and         r5,  r5,  r0            # DCT output = 0 (f_val)
    and         r6,  r6,  r0            # values added to output so far = 0 (i)

# Inner loop
    slli        r7,  r6,  3             # current row * 8 = index of start of row
    addu        r8,  r7,  r2            # r7 now contains the position in the block of the pixel we want
    slli        r8,  r8,  2             # shift to get a byte address
    addu        r8,  r8,  r30           # r8 now holds the location of the pixel in memory
    ldw         r8,  0        > 1       # load the pixel (perhaps use ldb?)
    ldw         r4,  0x10000  > 1       # load the weight for this pixel
    or          r8,  ch1, r0            # store whichever value arrives first
    mullw       r8,  ch1, r8            # multiply by the value which arrives second
    addu        r5,  r5,  r8            # add to the output
    addui       r6,  r6,  1             # increment values added
    seteqi.p    r0,  r6,  8             # set predicate if we've added 8 values
    addui       r4,  r4,  4             # move to the next weight (filling predicate delay slot)
    if!p?ibjmp  -96                     # restart loop if we're not finished

    setnei.p    r0,  r3,  0             # set predicate if we are not in the first row
    slli        r8,  r3,  3             # position of start of current row
    addu        r8,  r8,  r2            # position in block to write to
    slli        r8,  r8,  2             # shift so we have a byte position
    addu        r8,  r8,  r30           # memory location to write to
    ifp?or      r9,  r5,  r0            # if p, want to write sum relatively unchanged
    if!p?srai   r9,  r5,  14            # else, need to do some fancy tricks with the sum
    if!p?mullw  r9,  r9,  r29
    srai        r9,  r9,  15            # the shift is common to both branches
    stw         r9,  r8,  0x10000 > 1   # store the value

    addui       r3,  r3,  1             # increment current row
    seteqi.p    r0,  r3,  8             # set predicate if we've finished all 8 rows in this column
    if!p?ibjmp  -216                    # jump back to start of loop (f_val = 0)

    addui       r2,  r2,  1             # increment current column
    seteqi.p    r0,  r2,  8             # set predicate if we've finished all 8 columns
    if!p?ibjmp  -256                    # jump back to start of loop (row = 0)

    syscall.eop 1
