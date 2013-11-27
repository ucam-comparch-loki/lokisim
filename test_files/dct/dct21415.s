_start:
#    fetch           r0,  tocore1
	ori r11, r0, 4096
	ori r12, r0, 8192
	ori r13, r0, 4608
	ori r14, r0, 8704
	setchmap 1, r11
	setchmap 2, r12
	setchmap 3, r13
	setchmap 4, r14
	or r0, r3, r0 -> 3
	or r0, r3, r0 -> 4
    fetch.eop       r0,  tocore1

tocore1:
#    fetch           r0,  tocore2
	rmtexecute -> 1
	ifp?addui r4, r16, 512
	ifp?addui r5, r4, 256
	ifp?ori r9, r0, 8704
	ifp?setchmap 0, r4
	ifp?setchmap 1, r5
	ifp?ori r0, r0, 16777216 -> 0
	ifp?ori r0, r0, 4352 -> 0
	ifp?ori r0, r0, 16777216 -> 1
	ifp?ori r0, r0, 4864 -> 1
	ifp?setchmap 3, r9
	ifp?or r0, r17, r0
    ifp?fetch       r0,  core1
	ori r0, r0, 66048 -> 3
	ori r0, r0, 131072 -> 3
    fetch.eop       r0,  tocore2

tocore2:
	rmtexecute -> 2
	ifp?addui r4, r16, 1024
	ifp?addui r5, r4, 256
	ifp?setchmap 0, r4
	ifp?setchmap 1, r5
	ifp?ori r0, r0, 16777216 -> 0
	ifp?ori r0, r0, 8448 -> 0
	ifp?ori r0, r0, 16777216 -> 1
	ifp?ori r0, r0, 8960 -> 1
	ifp?or r0, r17, r0
    ifp?fetch       r0,  core2
	or.eop r0, r0, r0


# 1D DCT along rows

# Prologue: connect to data memories
core1:
	or r30, r16, r0
	or r31, r16, r0
	ori r29, r0, 11585

# Initialisation: receive parameters
	and r2, r2, r0
	and r3, r3, r0
	and r4, r4, r0
	and r5, r5, r0
	and r6, r6, r0

# Inner loop
	slli r7, r2, 3
	addu r8, r7, r6
	slli r8, r8, 2
	addu r8, r8, r30
	ldw 0(r8) -> 1
	ldw 65536(r4) -> 1
	or r8, r17, r0
	mullw r8, r17, r8
	addu r5, r5, r8
	addui r6, r6, 1
	seteqi.p r0, r6, 8
	addui r4, r4, 4
	if!p?ibjmp -96

	setnei.p r0, r3, 0
	addu r8, r7, r3
	slli r8, r8, 2
	addu r8, r8, r31
	ifp?or r9, r5, r0
	if!p?srai r9, r5, 14
	if!p?mullw r9, r9, r29
	srai r9, r9, 15
	stw r9, 0(r8) -> 1

	addui r3, r3, 1
	seteqi.p r0, r3, 8
	if!p?ibjmp -208

	addui r2, r2, 1
	seteqi.p r0, r2, 8
	or r0, r0, r0
	if!p?ibjmp -256

	or.eop r0, r31, r0 -> 3


# 1D DCT along columns
# Note: instead of using this, the results of the first 1D DCT could be
# transposed, and fed back into the same 1D DCT.
# This might save a little code, but would probably use more cores, without
# speeding up execution.

# Prologue: connect to data memories
core2:
	ori r29, r0, 11585

# Initialisation: receive parameters
	or r30, r16, r0

	and r2, r2, r0
	and r3, r3, r0
	and r4, r4, r0
	and r5, r5, r0
	and r6, r6, r0

# Inner loop
	slli r7, r6, 3
	addu r8, r7, r2
	slli r8, r8, 2
	addu r8, r8, r30
	ldw 0(r8) -> 1
	ldw 65536(r4) -> 1
	or r8, r17, r0
	mullw r8, r17, r8
	addu r5, r5, r8
	addui r6, r6, 1
	seteqi.p r0, r6, 8
	addui r4, r4, 4
	if!p?ibjmp -96

	setnei.p r0, r3, 0
	slli r8, r3, 3
	addu r8, r8, r2
	slli r8, r8, 2
	addu r8, r8, r30
	ifp?or r9, r5, r0
	if!p?srai r9, r5, 14
	if!p?mullw r9, r9, r29
	srai r9, r9, 15
	stw r9, 65536(r8) -> 1

	addui r3, r3, 1
	seteqi.p r0, r3, 8
	if!p?ibjmp -216

	addui r2, r2, 1
	seteqi.p r0, r2, 8
	if!p?ibjmp -256

	syscall.eop 1

