# Connect to memory.
_start:
#    ori                 r5,  r0,  (8,1)
    ori                 r5,  r0,  (0,0,8,0)
    setchmap            1,   r5
#    ori                 r0,  r0,  (0,2)  > 1

    addui               r3,  r0,  1000          # The address to store to.
    addui               r2,  r0,  1337          # The number to store.

# Send a stream of stores to memory to make sure none go missing.
# Note that reading this instruction packet has the side effect of testing
# many consecutive read operations.
    stw                 r2,  r3,  0      > 1
    stw                 r2,  r3,  4      > 1
    stw                 r2,  r3,  8      > 1
    stw                 r2,  r3,  12     > 1
    stw                 r2,  r3,  16     > 1
    stw                 r2,  r3,  20     > 1
    stw                 r2,  r3,  24     > 1
    stw                 r2,  r3,  28     > 1
    stw                 r2,  r3,  32     > 1
    stw                 r2,  r3,  36     > 1
    stw                 r2,  r3,  40     > 1
    stw                 r2,  r3,  44     > 1
    stw                 r2,  r3,  48     > 1
    stw                 r2,  r3,  52     > 1
    stw                 r2,  r3,  56     > 1
    stw                 r2,  r3,  60     > 1
    stw                 r2,  r3,  64     > 1
    stw                 r2,  r3,  68     > 1
    stw                 r2,  r3,  72     > 1
    stw                 r2,  r3,  76     > 1
    stw                 r2,  r3,  80     > 1
    stw                 r2,  r3,  84     > 1
    stw                 r2,  r3,  88     > 1
    stw                 r2,  r3,  92     > 1
    stw                 r2,  r3,  96     > 1
    stw                 r2,  r3,  100    > 1
    stw                 r2,  r3,  104    > 1
    stw                 r2,  r3,  108    > 1
    stw                 r2,  r3,  112    > 1
    stw                 r2,  r3,  116    > 1
    stw                 r2,  r3,  120    > 1
    stw                 r2,  r3,  124    > 1

# A quick test of the remaining memory operations.
    ldw                 r3,  0           > 1
    ldbu                r3,  0           > 1
    ldbu                r3,  1           > 1
    ori                 r5,  ch0, r0
    ori                 r6,  ch0, r0
    ori                 r7,  ch0, r0
    stb                 r7,  r3,  -3     > 1
    stb                 r6,  r3,  -4     > 1
    syscall.eop         1
