# SIMD framework: insert this code before the main program, and the core to
# access it will become the control core, and distribute the work across the
# given number of SIMD cores.
#
# Assumes that the number of cores to use is in memory 13 at address 0.
#
# r30 and r31 hold the SIMD parameters---the core's position in the SIMD vector
# and the total number of members, respectively---so probably should not be
# overwritten by the SIMD program.

_start:
	ldw 0(r0) -> 1
#    fetch               r0,  setuploop

	or r10, r16, r0
	addui r7, r10, -1
	ori r8, r0, 4096
	mullw r5, r7, r8
	addui r6, r5, 1792
    fetch.eop           r0,  setuploop

# Set up connections to remote core
setuploop:
	setchmap 2, r5
	setchmap 3, r6

# Send core any parameters it needs
	ori r0, r7, 0 -> 3
	ori r0, r3, 0 -> 3
	ori r0, r10, 0 -> 3

# Set up remote core's channel map table and fetch program.
# Note that we need the IPK FIFO to be at least as long as this code sequence.
# TODO: turn this into an instruction packet (or two), and put it in one core's
# cache (using fill, rather than fetch). Then have the whole group gfetch it.
	rmtexecute -> 2
	ifp?or r30, r21, r0
	ifp?ori r5, r0, 256
	ifp?mullw r6, r5, r30
	ifp?slli r6, r6, 1
	ifp?addu r3, r21, r6
	ifp?addu r4, r3, r5
	ifp?ori r8, r0, 4096
	ifp?mullw r8, r8, r30
	ifp?setchmap 0, r3
	ifp?ori r0, r0, 16777216 -> 0
	ifp?addui r0, r8, 256 -> 0
	ifp?setchmap 1, r4
	ifp?ori r0, r0, 16777216 -> 1
	ifp?addui r0, r8, 512 -> 1
	ifp?or r31, r21, r0
	ifp?or r0, r16, r0
	ifp?fetch r0, 4384

# If more members, loop
	addui r7, r7, -1
	setgte.p r0, r7, r0
	ifp?subu r5, r5, r8
	ifp?subu r6, r6, r8
	ifp?ibjmp -216
	or.eop r0, r0, r0

