_start:
    fetchr          tocore1
    lli             r11, (0,1,0)
    lli             r12, (0,2,0)
    lli             r13, (0,1,2)
    lli             r14, (0,2,2)
    setchmapi       1,   r11                # map 1 = cluster 1 instructions
    setchmapi       2,   r12                # map 2 = cluster 2 instructions
    setchmapi       3,   r13                # map 3 = cluster 1 data
    setchmapi       4,   r14                # map 4 = cluster 2 data
    addu            r0,  r20, r0  -> 3      # send cache configuration to core 1
    addu.eop        r0,  r20, r0  -> 4      # send cache configuration to core 2

tocore1:
    fetchr          tocore2
    rmtexecute                    -> 1      # start sending instructions to cluster 1
    ifp?lli         r24, (0,0,2,0,0)
    ifp?lli         r25, (0,0,1,0,0)
    ifp?addu        r24, ch0, r24           # receive/modify cache configuration...
    ifp?addu        r25, r24, r25           # ...for the channels we will connect to
    ifp?lli         r29, (0,2,4)
    ifp?setchmapi   0,   r24                # instruction memory connection = map 0
    ifp?setchmapi   1,   r25                # data memory connection = map 1
    ifp?lui         r9,  0x0100
    ifp?addu        r0,  r9,  r0      -> 0  # tell memory we're about to connect
    ifp?lli         r8,  (0,1,1)
    ifp?addu        r0,  r8,  r0      -> 0  # connect to instruction channel
    ifp?addu        r0,  r9,  r0      -> 1  # tell memory we're about to connect
    ifp?lli         r8,  (0,1,3)
    ifp?addu        r0,  r8,  r0      -> 1  # connect to data channel
    ifp?setchmapi   3,   r29                # next core in pipeline = map 3
    ifp?addu        r0,  ch1, r0            # receive sync message from memory
    ifp?fetch       ch0

    lli             r8,  %lo(core1)
    addu            r0,  r8,  r0      -> 3  # Tell Cluster 1 where its first IPK is
    lli             r8,  %lo(0x10200)
    lui             r8,  %hi(0x10200)
    addu            r0,  r8,  r0      -> 3  # Tell Cluster 1 where its input is
    lli             r8,  %lo(0x20000)
    lui             r8,  %hi(0x20000)
    addu.eop        r0,  r8,  r0      -> 3  # Tell Cluster 1 where to store its output

tocore2:
    rmtexecute                        -> 2  # start sending instructions to cluster 2
    ifp?lli         r24, (0,0,4,0,0)
    ifp?lli         r25, (0,0,1,0,0)
    ifp?addu        r24, ch0, r24           # receive/modify cache configuration...
    ifp?addu        r25, r24, r25           # ...for the channels we will connect to
    ifp?setchmapi   0,   r24                # instruction memory connection = map 0
    ifp?setchmapi   1,   r25                # data memory connection = map 1
    ifp?lui         r9,  0x0100
    ifp?addu        r0,  r9,  r0      -> 0  # tell memory we're about to connect
    ifp?lli         r8,  (0,2,1)
    ifp?addu        r0,  r8,  r0      -> 0  # connect to instruction channel
    ifp?addu        r0,  r9,  r0      -> 1  # tell memory we're about to connect
    ifp?lli         r8,  (0,2,3)
    ifp?addu        r0,  r8,  r0      -> 1  # connect to data channel
    ifp?addu        r0,  ch1, r0            # receive sync message from memory
    ifp?fetch       ch0

    lli             r8,  %lo(core2)
    addu            r0,  r8,  r0      -> 4  # Tell cluster 2 where its first IPK is
    lli             r8,  %lo(0x30000)
    lui             r8,  %hi(0x30000)
    addu.eop        r0,  r8,  r0      -> 4  # Tell cluster 2 where to store its output


# 1D DCT along rows

# Prologue: connect to data memories
core1:
    addu        r30, ch0, r0            # wait for location of input block
    addu        r31, ch0, r0            # wait for location of output block
    lli         r29, 11585              # store a value we need

# Initialisation: receive parameters
    addu        r12, r0,  r0            # current row = 0
    addu        r13, r0,  r0            # current column = 0
    lli         r14, %lo(0x10000)
    lui         r14, %hi(0x10000)       # index of current weight = 0 (k)
    addu        r15, r0,  r0            # DCT output = 0 (f_val)
    addu        r16, r0,  r0            # values added to output so far = 0 (i)

# Inner loop
    slli        r17, r12, 3             # current row * 8 = index of start of row
    addu        r18, r17, r16           # r17 now contains the position in the block of the pixel we want
    slli        r18, r18, 2             # shift to get a byte address
    addu        r18, r18, r30           # r18 now holds the location of the pixel in memory
    ldw         0(r18)        -> 1      # load the pixel (perhaps use ldb?)
    addu        r18, ch1, r0            # store whichever value arrives first
    ldw         0(r14)        -> 1      # load the weight for this pixel
    mullw       r18, ch1, r18           # multiply by the value which arrives second
    addu        r15, r15, r18           # add to the output
    addui       r16, r16, 1             # increment values added
    seteqi.p    r0,  r16, 8             # set predicate if we've added 8 values
    addui       r14, r14, 4             # move to the next weight (filling predicate delay slot)
    if!p?ibjmp  -48                     # restart loop if we're not finished

    setnei.p    r0,  r13, 0             # set predicate if we are not in the first column
    addu        r18, r17, r13           # position in block to write to
    slli        r18, r18, 2             # shift so we get a byte position
    addu        r18, r18, r31           # memory location to write to
    ifp?addu    r19, r15, r0            # if p, want to write sum relatively unchanged
    if!p?srai   r19, r15, 14            # else, need to do some fancy tricks with the sum
    if!p?mullw  r19, r19, r29
    srai        r19, r19, 15            # the shift is common to both branches
    stw         r19, 0(r18)   -> 1      # store the value

    addui       r13, r13, 1             # increment current column
    seteqi.p    r0,  r13, 8             # set predicate if we've finished all 8 columns in this row
    if!p?ibjmp  -104                    # jump back to start of loop (f_val = 0)

    addui       r12, r12, 1             # increment current row
    seteqi.p    r0,  r12, 8             # set predicate if we've finished all 8 rows
    if!p?ibjmp  -128                    # jump back to start of loop (column = 0)

    addu.eop    r0,  r31, r0  -> 3      # send a pointer to the output block to the next core


# 1D DCT along columns
# Note: instead of using this, the results of the first 1D DCT could be
# transposed, and fed back into the same 1D DCT.
# This might save a little code, but would probably use more cores, without
# speeding up execution.

# Prologue: connect to data memories
core2:
    lli         r29, 11585              # store a value we need

# Initialisation: receive parameters
    addu        r31, ch0, r0            # wait for location of output block
    addu        r30, ch2, r0            # wait for location of input block

    addu        r12, r0,  r0            # current column = 0
    addu        r13, r0,  r0            # current row = 0
    lli         r14, %lo(0x10000)
    lui         r14, %hi(0x10000)       # index of current weight = 0 (k)
    addu        r15, r0,  r0            # DCT output = 0 (f_val)
    addu        r16, r0,  r0            # values added to output so far = 0 (i)

# Inner loop
    slli        r17, r16, 3             # current row * 8 = index of start of row
    addu        r18, r17, r12           # r17 now contains the position in the block of the pixel we want
    slli        r18, r18, 2             # shift to get a byte address
    addu        r18, r18, r30           # r18 now holds the location of the pixel in memory
    ldw         0(r18)       -> 1       # load the pixel (perhaps use ldb?)
    addu        r18, ch1, r0            # store whichever value arrives first
    ldw         0(r14)       -> 1       # load the weight for this pixel
    mullw       r18, ch1, r18           # multiply by the value which arrives second
    addu        r15, r15, r18           # add to the output
    addui       r16, r16, 1             # increment values added
    seteqi.p    r0,  r16, 8             # set predicate if we've added 8 values
    addui       r14, r14, 4             # move to the next weight (filling predicate delay slot)
    if!p?ibjmp  -48                     # restart loop if we're not finished

    setnei.p    r0,  r13, 0             # set predicate if we are not in the first row
    slli        r18, r13, 3             # position of start of current row
    addu        r18, r18, r12           # position in block to write to
    slli        r18, r18, 2             # shift so we have a byte position
    addu        r18, r18, r31           # memory location to write to
    ifp?addu    r19, r15, r0            # if p, want to write sum relatively unchanged
    if!p?srai   r19, r15, 14            # else, need to do some fancy tricks with the sum
    if!p?mullw  r19, r19, r29
    srai        r19, r19, 15            # the shift is common to both branches
    stw         r19, 0(r18)  -> 1       # store the value

    addui       r13, r13, 1             # increment current row
    seteqi.p    r0,  r13, 8             # set predicate if we've finished all 8 rows in this column
    if!p?ibjmp  -108                    # jump back to start of loop (f_val = 0)

    addui       r12, r12, 1             # increment current column
    seteqi.p    r0,  r12, 8             # set predicate if we've finished all 8 columns
    if!p?ibjmp  -132                    # jump back to start of loop (row = 0)

    syscall.eop 1
