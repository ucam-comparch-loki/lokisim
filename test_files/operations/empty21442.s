# The bootloader may end with a fetch command, but some of these tests execute
# one instruction at a time, so don't get their instructions from memory.
# Load this "program" so that the memory doesn't keep loading empty words
# forever.

	or.eop r0, r0, r0

