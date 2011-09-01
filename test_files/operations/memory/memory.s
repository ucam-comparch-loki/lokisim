# Connect to memory.
_start:
    addui               r3,  r0,  1000          # The address to store to.
    addui               r2,  r0,  1337          # The number to store.

# Send a stream of stores to memory to make sure none go missing.
# Note that reading this instruction packet has the side effect of testing
# many consecutive read operations.
    stw                 r2,  0(r3)      -> 1
    stw                 r2,  4(r3)      -> 1
    stw                 r2,  8(r3)      -> 1
    stw                 r2,  12(r3)     -> 1
    stw                 r2,  16(r3)     -> 1
    stw                 r2,  20(r3)     -> 1
    stw                 r2,  24(r3)     -> 1
    stw                 r2,  28(r3)     -> 1
    stw                 r2,  32(r3)     -> 1
    stw                 r2,  36(r3)     -> 1
    stw                 r2,  40(r3)     -> 1
    stw                 r2,  44(r3)     -> 1
    stw                 r2,  48(r3)     -> 1
    stw                 r2,  52(r3)     -> 1
    stw                 r2,  56(r3)     -> 1
    stw                 r2,  60(r3)     -> 1
    stw                 r2,  64(r3)     -> 1
    stw                 r2,  68(r3)     -> 1
    stw                 r2,  72(r3)     -> 1
    stw                 r2,  76(r3)     -> 1
    stw                 r2,  80(r3)     -> 1
    stw                 r2,  84(r3)     -> 1
    stw                 r2,  88(r3)     -> 1
    stw                 r2,  92(r3)     -> 1
    stw                 r2,  96(r3)     -> 1
    stw                 r2,  100(r3)    -> 1
    stw                 r2,  104(r3)    -> 1
    stw                 r2,  108(r3)    -> 1
    stw                 r2,  112(r3)    -> 1
    stw                 r2,  116(r3)    -> 1
    stw                 r2,  120(r3)    -> 1
    stw                 r2,  124(r3)    -> 1

# A quick test of the remaining memory operations.
    ldw                 0(r3)           -> 1
    ldbu                0(r3)           -> 1
    ldbu                1(r3)           -> 1
    ori                 r5,  ch0, r0
    ori                 r6,  ch0, r0
    ori                 r7,  ch0, r0
    stb                 r7,  -3(r3)     -> 1
    stb                 r6,  -4(r3)     -> 1
    syscall.eop         1
