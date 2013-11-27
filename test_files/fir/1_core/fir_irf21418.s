# Very similar to default code, but makes use of indirect register file to
# cache the taps. We can have no more than 32 taps.

_start:
#    fetch           r0,  loadtaps           # get the next instruction packet

# Load the parameters for this filter. (May deadlock if buffers are small?)
	ldw 4(r0) -> 1
	ldw 8(r0) -> 1
	ldw 12(r0) -> 1
	ldw 16(r0) -> 1
	ldw 20(r0) -> 1

# Store the parameters locally.
	ori r10, r16, 0
	ori r11, r16, 0
	ori r12, r16, 0
	ori r13, r16, 0
	ori r14, r16, 0

# Compute end point (start point + 4*(input length + num taps - 1))
	addui r26, r10, -1
	slli r26, r26, 2
	slli r12, r12, 2
	addu r29, r13, r12
	addu r27, r29, r26
	addui r27, r27, -4
	addu r28, r11, r26

	ori r4, r13, 0
    fetch.eop       r0,  loadtaps

# Load taps into indirect register file.
loadtaps:
#    fetch           r0,  loop
	ori r5, r0, 32
	ldw 0(r11) -> 1
	addui r11, r11, 4
	iwtr r5, r16
	addui r5, r5, 1
	setgteu.p r0, r28, r11
	ifp?ibjmp -40
	addui r8, r10, 32
    fetch.eop       r0,  loop

# Start of outer loop
loop:
	ori r9, r0, 0
	subu r6, r4, r26
	ori r5, r0, 32

# Start of inner loop
	setgteu.p r0, r6, r13
	ifp?setltu.p r0, r6, r29
	ifp?ldw 0(r6) -> 1
	ifp?irdr r7, r5
	addui r5, r5, 1
	ifp?mullw r7, r16, r7
	ifp?addu r9, r9, r7

	setlt.p r0, r5, r8
	addui r6, r6, 4
	ifp?ibjmp -72
# End of inner loop

	addui r4, r4, 4
	stw r9, 0(r14) -> 1
	setgteu.p r0, r27, r4
	addui r14, r14, 4
	ifp?ibjmp -136
# End of outer loop

	syscall.eop 1

