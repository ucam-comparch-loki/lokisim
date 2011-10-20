# Connect to memory.
_start:
    lli                 r13,  r0,  1000          # The address to store to.
    lli                 r12,  r0,  1337          # The number to store.

# Send a stream of stores to memory to make sure none go missing.
# Note that reading this instruction packet has the side effect of testing
# many consecutive read operations.
    stw                 r12,  0(r13)      -> 1
    stw                 r12,  4(r13)      -> 1
    stw                 r12,  8(r13)      -> 1
    stw                 r12,  12(r13)     -> 1
    stw                 r12,  16(r13)     -> 1
    stw                 r12,  20(r13)     -> 1
    stw                 r12,  24(r13)     -> 1
    stw                 r12,  28(r13)     -> 1
    stw                 r12,  32(r13)     -> 1
    stw                 r12,  36(r13)     -> 1
    stw                 r12,  40(r13)     -> 1
    stw                 r12,  44(r13)     -> 1
    stw                 r12,  48(r13)     -> 1
    stw                 r12,  52(r13)     -> 1
    stw                 r12,  56(r13)     -> 1
    stw                 r12,  60(r13)     -> 1
    stw                 r12,  64(r13)     -> 1
    stw                 r12,  68(r13)     -> 1
    stw                 r12,  72(r13)     -> 1
    stw                 r12,  76(r13)     -> 1
    stw                 r12,  80(r13)     -> 1
    stw                 r12,  84(r13)     -> 1
    stw                 r12,  88(r13)     -> 1
    stw                 r12,  92(r13)     -> 1
    stw                 r12,  96(r13)     -> 1
    stw                 r12,  100(r13)    -> 1
    stw                 r12,  104(r13)    -> 1
    stw                 r12,  108(r13)    -> 1
    stw                 r12,  112(r13)    -> 1
    stw                 r12,  116(r13)    -> 1
    stw                 r12,  120(r13)    -> 1
    stw                 r12,  124(r13)    -> 1

# A quick test of the remaining memory operations.
    ldw                 0(r13)           -> 1
    ldbu                0(r13)           -> 1
    ldbu                1(r13)           -> 1
    addu                r15,  ch0, r0
    addu                r16,  ch0, r0
    addu                r17,  ch0, r0
    stb                 r17,  -3(r13)     -> 1
    stb                 r16,  -4(r13)     -> 1
    syscall.eop         1
