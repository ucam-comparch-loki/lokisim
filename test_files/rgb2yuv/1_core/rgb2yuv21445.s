# RGB to YUV (technically YCbCr) colour space conversion
#
# Reads RGB data from three separate arrays, and outputs YUV data to three
# separate arrays. Each value is one byte long.
#
#   Y = (( 66R + 129G +  25B + 128) >> 8) + 16
#   U = ((-38R -  74G + 112B + 128) >> 8) + 128
#   V = ((112R -  94G -  18B + 128) >> 8) + 128

# Load parameters
_start:
#    fetch               r0,  load
	ldw 4(r0) -> 1
	ldw 8(r0) -> 1
	ori r2, r16, 0
	ori r3, r16, 0
	mullw r4, r3, r2

# Initialisation
	ori r10, r0, 0
    fetch.eop           r0,  load

# Start of loop
# Load subpixel values
load:
#    fetch               r0,  computey
	ldbu 65536(r10) -> 1
	ldbu 69632(r10) -> 1
	ldbu 73728(r10) -> 1
	ori r7, r16, 0
	ori r8, r16, 0
	ori r9, r16, 0
    fetch.eop           r0,  computey

# Compute Y
computey:
#    fetch               r0,  compute
	ori r25, r0, 66
	ori r26, r0, 129
	ori r27, r0, 25
    addui               r29, r0,  computeu      # store next IPK location
    fetch.eop           r0,  compute

# Store Y, compute U
computeu:
	addui r25, r25, 16
	stb r25, 131072(r10) -> 1
#    fetch               r0,  compute
	ori r25, r0, -38
	ori r26, r0, -74
	ori r27, r0, 112
    addui               r29, r0,  computev      # store next IPK location
    fetch.eop           r0,  compute

# Store U, compute V
computev:
	addui r25, r25, 128
	stb r25, 135168(r10) -> 1
#    fetch               r0,  compute
	ori r25, r0, 112
	ori r26, r0, -94
	ori r27, r0, -18
    addui               r29, r0,  endofloop     # store next IPK location
    fetch.eop           r0,  compute

# Store V, loop if necessary
endofloop:
	addui r25, r25, 128
	stb r25, 139264(r10) -> 1
	addui r10, r10, 1
	setlt.p r0, r10, r4
    ori                 r25, r0,  load
    ori                 r26, r0,  exit
	psel.fetch.eop r25, r26

exit:
	syscall.eop 1

# Subroutine to compute (xR + yG + zB + 128) >> 8
# Excludes final addition because it is more efficiently done with addui.
# Assumes x is in r25, y is in r26, z is in r27 and next IPK address is in r29.
# Leaves result in r25.
compute:
	mullw r25, r25, r7
	mullw r26, r26, r8
	mullw r27, r27, r9
#    fetch               r29, 0
	addu r25, r25, r26
	addu r25, r25, r27
	addui r25, r25, 128
	srli r25, r25, 8
	fetch.eop r29, 0

