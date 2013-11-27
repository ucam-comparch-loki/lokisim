# Load parameters
simdstart:
#    fetch               r0,  loadrow
	ldw 4(r0) -> 1
	ldw 8(r0) -> 1
	ldw 12(r0) -> 1
	ori r2, r16, 0
	ori r3, r16, 0
	ori r4, r16, 0
	ori r11, r30, 0
    ori                 r25, r0,  loadrow
    ori                 r26, r0,  end           # some branch addresses for later
    fetch.eop           r0,  loadrow

# Load row of matrix 1 into IRF
loadrow:
#    fetch               r0,  loop
	ori r10, r0, 32
	addu r12, r10, r3
	mullw r5, r3, r11
	slli r5, r5, 2
	ldw 65536(r5) -> 1
	addui r5, r5, 4
	iwtr r10, r16
	addui r10, r10, 1
	setlt.p r0, r10, r12
	ifp?ibjmp -40
    fetch.eop           r0,  loop

# Start of main loop
loop:
	ori r12, r0, 0
	ori r13, r0, 0
	ori r14, r0, 0

# Compute where to load from in matrix 2
	mullw r6, r4, r14
	addu r6, r6, r12
	slli r6, r6, 2
	ldw 131072(r6) -> 1

# Retrieve matrix 1 value from IRF
	addui r7, r14, 32
	addui r14, r14, 1
	irdr r7, r7

# Deal with inputs (and loop if necessary)
	seteq.p r0, r14, r3
	mullw r7, r7, r16
	addu r13, r13, r7
	if!p?ibjmp -80

# Compute where to store the result
	mullw r7, r11, r4
	addu r7, r7, r12
	slli r7, r7, 2
	stw r13, 196608(r7) -> 1

	addui r12, r12, 1
	seteq.p r0, r12, r4
	if!p?ibjmp -152

	addu r11, r11, r31
	setgte.p r0, r11, r2
	psel.fetch.eop r26, r25

end:
	seteq.p r0, r30, r0
	ifp?syscall 1
	or.eop r0, r0, r0

