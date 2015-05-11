OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = 0x00100000, LENGTH = 0x00008000
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init)
		*(.text)
		*(.rodata)
	}
}

